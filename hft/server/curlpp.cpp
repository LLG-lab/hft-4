/**********************************************************************\
**                                                                    **
**             -=≡≣ High Frequency Trading System ® ≣≡=-              **
**                                                                    **
**          Copyright © 2017 - 2026 by LLG Ryszard Gradowski          **
**                       All Rights Reserved.                         **
**                                                                    **
**  CAUTION! This application is an intellectual property             **
**           of LLG Ryszard Gradowski. This application as            **
**           well as any part of source code cannot be used,          **
**           modified and distributed by third party person           **
**           without prior written permission issued by               **
**           intellectual property owner.                             **
**                                                                    **
\**********************************************************************/

#include <iostream>
#include <cstring>
#include <cerrno>
#include <sstream>

#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <curlpp.hpp>
#include "../hft-config.h"


#define CURL_SETOPT_SAFE(__OPTION__, __PARAMETER__) \
           curl_easy_setopt_chk(curl_easy_setopt(curl_ctx_, \
                                              __OPTION__, \
                                              __PARAMETER__), \
                             #__OPTION__);

bool curlpp::curl_global_initialized_ = false;

std::mutex curlpp::init_global_mtx_;

void curlpp::init_globals(void)
{
    if (! curlpp::curl_global_initialized_)
    {
        //
        // This part of initialization
        // have to be done once.
        //

        if (curl_global_init(CURL_GLOBAL_DEFAULT) != 0)
        {
            throw std::runtime_error("Unable to initialize curl library");
        }

        curlpp::curl_global_initialized_ = true;
    }
}

curlpp::curlpp(void)
    : curl_ctx_ {nullptr}
{
    memset(curl_error_buffer_, 0, 1024);

    //
    // Initialize curl library.
    //

    try
    {
        std::lock_guard<std::mutex> lck(init_global_mtx_);

        init_globals();
    }
    catch (std::runtime_error &e)
    {
        throw e;
    }

    //
    // Per-object initialization part.
    //

    curl_ctx_ = curl_easy_init();

    if (curl_ctx_ == nullptr)
    {
        //
        // Uninit curl library.
        //

        destroy_curl();

        throw curlpp_init_error("Unable to initialize curl context");
    }

    //
    // General setup of curl context.
    //

    std::ostringstream user_agent_name;

    user_agent_name << "HFT server for linux, version "
                    << HFT_VERSION_MAJOR << "." << HFT_VERSION_MINOR;

    CURL_SETOPT_SAFE(CURLOPT_ERRORBUFFER, curl_error_buffer_);
    CURL_SETOPT_SAFE(CURLOPT_NOSIGNAL, 1)
    CURL_SETOPT_SAFE(CURLOPT_USERAGENT, user_agent_name.str().c_str())
    CURL_SETOPT_SAFE(CURLOPT_WRITEDATA, this)
    CURL_SETOPT_SAFE(CURLOPT_WRITEFUNCTION, &curl_write_data_callback)
    CURL_SETOPT_SAFE(CURLOPT_CONNECTTIMEOUT, 5)
    CURL_SETOPT_SAFE(CURLOPT_SSL_VERIFYPEER, false)
    CURL_SETOPT_SAFE(CURLOPT_PROXY, "")

    //
    // Initial capacity for transfer buffer.
    //

    transfer_buffer_.reserve(4096);
}

curlpp::~curlpp(void)
{
    destroy_curl();
}

void curlpp::download_remote_target(const std::string &uri,
                                        const std::string &file_name,
                                            const headers_list &custom_headers)
{
    transfer_buffer_.clear();
    struct curl_slist *list = nullptr;

    if (! custom_headers.empty())
    {
        for (auto &header : custom_headers)
        {
            list = curl_slist_append(list, header.c_str());
        }
    }

    CURL_SETOPT_SAFE(CURLOPT_URL, uri.c_str())
    CURL_SETOPT_SAFE(CURLOPT_HTTPGET, 1)
    CURL_SETOPT_SAFE(CURLOPT_HTTPHEADER, list)

    CURLcode status = curl_easy_perform(curl_ctx_);

    if (list != nullptr)
    {
        curl_slist_free_all(list);
        list = nullptr;
    }

    //
    // Check for connection error.
    //

    if (status != CURLE_OK)
    {
        std::ostringstream err_msg;

        err_msg << "Failed to transfer data. Error code: " << status
                << " ‘" << curl_error_buffer_ << "’";

        throw curlpp_transfer_error(err_msg.str());
    }

    //
    // Check HTTP response code, if fail throw exception too.
    //

    long http_code = 0;

    curl_easy_getinfo(curl_ctx_, CURLINFO_RESPONSE_CODE, &http_code);

    if (http_code != 200)
    {
        std::ostringstream err_msg;

        err_msg << "Transfer error: Got HTTP response code " << http_code;

        throw curlpp_transfer_error(err_msg.str());
    }

    //
    // If file name is not specified,
    // data will not be stored on the disk.
    //

    if (file_name.length() == 0)
    {
        return;
    }

    //
    // If downloaded successfully,
    // write data to the file.
    //

    FILE *pFile;
    pFile = fopen(file_name.c_str(), "wb");
    int e = errno;

    if (pFile == nullptr)
    {
        std::ostringstream err_msg;

        err_msg << "Unable to write file: ‘" << file_name
                << "’ error is " << e;

        throw curlpp_transfer_error(err_msg.str());
    }

    int chunks = transfer_buffer_.size() / 1024;
    size_t s;

    for (int i = 0; i < chunks; i++)
    {
        s = fwrite(transfer_buffer_.c_str() + i*1024, 1, 1024, pFile);

        if (s != 1024)
        {
            e = errno;

            std::ostringstream err_msg;

            fclose(pFile);

            err_msg << "curlpp I/O error (" << e << ")";

            throw curlpp_transfer_error(err_msg.str());
        }
    }

    int remainder_size = transfer_buffer_.size() % 1024;

    if (remainder_size > 0)
    {
        s = fwrite(transfer_buffer_.c_str()+chunks*1024, 1, remainder_size, pFile);

        if (s != remainder_size)
        {
            e = errno;

            std::ostringstream err_msg;

            fclose(pFile);

            err_msg << "curlpp I/O error (" << e << ")";

            throw curlpp_transfer_error(err_msg.str());
        }
    }

    fflush(pFile);
    fclose(pFile);
}

const std::string &curlpp::get_buffer(void) const
{
    return transfer_buffer_;
}

void curlpp::clear_buffer(void)
{
    transfer_buffer_.clear();
}

void curlpp::destroy_curl(void)
{
    //
    // Destroy curl context.
    //

    if (curl_ctx_ != nullptr)
    {
        curl_easy_cleanup(curl_ctx_);
    }

    //
    // Uninit curl library.
    //
/*
XXX Nie wiemy, który wątek ma to wykonać.
    Pomysł taki, aby wykonywał to destruktor jakiegos
    statycznego obiektu globalnego.

    if (curl_initialized_)
    {
        curl_global_cleanup();

        curl_global_initialized_ = false;
    }
*/
}

void curlpp::curl_easy_setopt_chk(CURLcode curl_code, const char *message)
{
    if (curl_code == CURLE_OK)
    {
        return;
    }

    std::ostringstream err_msg;

    err_msg << "Failed ‘curl_easy_setopt’ with option ‘"
            << message << "’. Error code: " << curl_code
            << " – " << curl_error_buffer_;

    throw curlpp_setup_error(err_msg.str());
}

size_t curlpp::curl_write_data_callback(char *ptr, size_t size,
                                            size_t nmemb, void *dev_private)
{
    //
    // Regain reference to the object.
    //

    curlpp *self = reinterpret_cast<curlpp *>(dev_private);

    self -> transfer_buffer_ += std::string(ptr, size*nmemb);

    return size*nmemb;
}
