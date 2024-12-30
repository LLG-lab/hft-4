/**********************************************************************\
**                                                                    **
**             -=≡≣ High Frequency Trading System ® ≣≡=-              **
**                                                                    **
**          Copyright © 2017 - 2025 by LLG Ryszard Gradowski          **
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

package hft2ducascopy;

import java.io.*;
import java.net.*;
import java.util.*;
import java.sql.Timestamp;
import java.time.LocalDateTime;
import java.time.format.DateTimeFormatter;

import org.json.*;

public class HftConnector {

    private Socket clientSocket = null;
    private DataOutputStream outToServer = null;
    private BufferedReader inFromServer  = null;

    private ProxyBridgeConfig config = null;

    private static final DateTimeFormatter dtf = DateTimeFormatter.ofPattern("yyyy-MM-dd HH:mm:ss.000");

    public HftConnector(ProxyBridgeConfig cfg) throws Exception {
        config = cfg;

        //
        // Connec to to HFT.
        //

        int connect_attempt = 0;

        while (true) {
            try {
                clientSocket = new Socket(config.getHost(), config.getPort());
                outToServer  = new DataOutputStream(clientSocket.getOutputStream());
                inFromServer = new BufferedReader(new InputStreamReader(clientSocket.getInputStream()));
            } catch (IOException e) {
                if (++connect_attempt < 7) {
                    System.err.println("Socket error: " + e.getMessage() + ". Wait for retry...");

                    try {
                        Thread.sleep(1000);
                    } catch (InterruptedException f) {
                        System.err.println(f.getMessage());
                    }

                    continue;
                } else {
                    String error = "Failed to connect to HFT server after " +
                                   connect_attempt + " trials. Error is ‘" +
                                   e.getMessage() + "’. Giving up";

                    throw new Exception(error);
                }
            }

            break;
        }

        //
        // Initialize session.
        //

        JSONObject jo = new JSONObject();

        jo.put("method", "init");
        jo.put("sessid", config.getSessId());

        JSONArray ja = new JSONArray();

        for (String instr : config.getInstruments()) {
            ja.put(instr);
        }

        jo.put("instruments", ja);

        String json_str = sendRecvLine(jo.toString());
        System.out.println(json_str);

        HftResponse hftResponse = new HftResponse(json_str);

        if (hftResponse.isError()) {
            throw new Exception("Failed to initialte HFT session: ‘" + hftResponse.getErrorMessage() + "’");
        }
    }

    public void sync(String instrument, String id, long ts, boolean is_long, double price, int qty) throws Exception {
        JSONObject jo = new JSONObject();

        jo.put("method", "sync");
        jo.put("instrument", instrument);
        jo.put("id", id);
        jo.put("timestamp", getDateTimeFromTimestampStr(ts));

        if (is_long) {
            jo.put("direction", "LONG");
        } else {
            jo.put("direction", "SHORT");
        }

        jo.put("price", price);
        jo.put("qty", qty);

        String json_str = sendRecvLine(jo.toString());
        System.out.println(json_str);

        HftResponse hftResponse = new HftResponse(json_str);

        if (hftResponse.isError()) {
            throw new Exception("Failed to sync position: ‘" + hftResponse.getErrorMessage() + "’");
        }
    }

    public HftResponse tick(String instrument, double ask, double bid, double equity, double free_margin) throws Exception {
        JSONObject jo = new JSONObject();

        jo.put("method", "tick");
        jo.put("instrument", instrument);
        jo.put("timestamp", getDateTimeStr());
        jo.put("ask", ask);
        jo.put("bid", bid);
        jo.put("equity", equity);
        jo.put("free_margin", free_margin);

        String json_str = sendRecvLine(jo.toString());
        //System.out.println(json_str);

        HftResponse hftResponse = new HftResponse(json_str);

        return hftResponse;
    }

    public void open_notify(String instrument, String id, boolean status, double price) throws Exception {
        JSONObject jo = new JSONObject();

        jo.put("method", "open_notify");
        jo.put("instrument", instrument);
        jo.put("id", id);
        jo.put("status", status);
        jo.put("price", price);

        String json_str = sendRecvLine(jo.toString());
        System.out.println(json_str);

        HftResponse hftResponse = new HftResponse(json_str);
    }

    public void close_notify(String instrument, String id, boolean status, double price) throws Exception {
        JSONObject jo = new JSONObject();

        jo.put("method", "close_notify");
        jo.put("instrument", instrument);
        jo.put("id", id);
        jo.put("status", status);
        jo.put("price", price);

        String json_str = sendRecvLine(jo.toString());
        System.out.println(json_str);

        HftResponse hftResponse = new HftResponse(json_str);
    }

    private String sendRecvLine(String json_line) throws Exception {
        try {
            outToServer.writeBytes(json_line + "\n");
            String reply = inFromServer.readLine();

            return reply;
        } catch (IOException e) {
            throw new Exception("Connection to HFT server was lost: ‘" + e.getMessage() + "’");
        }
    }

    private static String getDateTimeStr() {
        LocalDateTime now = LocalDateTime.now();
        return dtf.format(now);
        //return "2019-10-26 20:45:31.000";
    }

    private static String getDateTimeFromTimestampStr(long ts) {
        Timestamp timestamp = new Timestamp(ts);
        LocalDateTime ldt  = timestamp.toLocalDateTime();
        return dtf.format(ldt);
    }
}
