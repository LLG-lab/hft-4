/**********************************************************************\
**                                                                    **
**             -=≡≣ High Frequency Trading System ® ≣≡=-              **
**                                                                    **
**          Copyright © 2017 - 2023 by LLG Ryszard Gradowski          **
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

#ifndef __CURLPP_HPP__
#define __CURLPP_HPP__

#include <curl/curl.h>
#include <custom_except.hpp>

#include <list>
#include <stdexcept>
#include <mutex>

class curlpp
{
public:

    //
    // The curlpp exceptions.
    //

    DEFINE_CUSTOM_EXCEPTION_CLASS(curlpp_init_error, std::runtime_error)
    DEFINE_CUSTOM_EXCEPTION_CLASS(curlpp_setup_error, std::runtime_error)
    DEFINE_CUSTOM_EXCEPTION_CLASS(curlpp_transfer_error, std::runtime_error)

    static void init_globals(void);

    typedef std::list<std::string> headers_list;

    //
    // Constructor.
    //

    explicit curlpp(void);

    curlpp(curlpp &) = delete;

    curlpp(curlpp &&) = delete;

    ~curlpp(void);

    void download_remote_target(const std::string &uri, const std::string &file_name = std::string(""),
                                    const headers_list &custom_headers = headers_list());

    const std::string &get_buffer(void) const;

    void clear_buffer(void);

private:

    //
    // Destroys all curl stuff.
    //

    void destroy_curl(void);

    //
    // Exception wrapper for curl_easy_setopt.
    //

    void curl_easy_setopt_chk(CURLcode curl_code, const char *message);

    //
    // Callback for write data used by CURL.
    //

    static size_t curl_write_data_callback(char *ptr, size_t size,
                                               size_t nmemb, void *dev_private);

    //
    // Private connection context used by curl.
    //

    CURL *curl_ctx_;

    char curl_error_buffer_[1024];
    static bool curl_global_initialized_;

    //
    // Buffer, where curl_write_data_callback stores
    // data transfered from remote server.
    //

    std::string transfer_buffer_;

    static std::mutex init_global_mtx_;
};

#endif /* __CURLPP_HPP__ */
