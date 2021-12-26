/**********************************************************************\
**                                                                    **
**             -=≡≣ High Frequency Trading System ® ≣≡=-              **
**                                                                    **
**          Copyright © 2017 - 2022 by LLG Ryszard Gradowski          **
**                       All Rights Reserved.                         **
**                                                                    **
**  CAUTION! This application is an intellectual propery              **
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

import org.json.*;

public class HftResponse {

    public class OpenPositionInfo {
        public boolean is_long = true;
        public String id = "";
        public int qty = 0;

        public OpenPositionInfo(boolean is_long, String id, int qty) {
            this.is_long = is_long;
            this.id = id;
            this.qty = qty;
        }
    }

    private boolean error_status = false;
    private String error_message = "";

    private Vector<String> positions_close_collection;
    private Vector<OpenPositionInfo> positions_open_collection;

    public HftResponse(String json_string) throws Exception {

        this.positions_close_collection = new Vector<String>();
        this.positions_open_collection  = new Vector<OpenPositionInfo>();

        try {
            JSONObject jo = new JSONObject(json_string);

            String status = jo.getString("status");

            if (status.equals("ack")) {
                return;
            } else if (status.equals("error")) {
                this.error_status  = true;
                this.error_message = jo.getString("message");
                return;
            } else if (status.equals("advice")) {
                JSONArray arr = jo.getJSONArray("operations");

                for (int i = 0; i < arr.length(); i++) {
                    JSONObject operation = arr.getJSONObject(i);

                    boolean direction;
                    String op = operation.getString("op");

                    if (op.equals("close")) {
                        positions_close_collection.add(operation.getString("id"));
                        continue;
                    } else if (op.equals("LONG")) {
                        direction = true;
                    } else if (op.equals("SHORT")) {
                        direction = false;
                    } else {
                        String error = "Illegal operation for advice status from hft server: ‘" +
                                       op + "’";

                        throw new Exception(error);
                    }

                    String id = operation.getString("id");
                    int qty   = operation.getInt("qty");

                    positions_open_collection.add(new OpenPositionInfo(direction, id, qty));
                }
            } else {
                String error = "Illegal status from HFT server ‘" +
                               status + "’";

                throw new Exception(error);
            }
        } catch (JSONException e) {
            String error = "Invalid response from HFT server ‘" +
                           e.getMessage() + "’";

            throw new Exception(error);
        }
    }

    public boolean isError() {
        return error_status;
    }

    public String getErrorMessage() {
        return error_message;
    }

    public Vector<String> getPositionsCloseCollection() {
        return positions_close_collection;
    }

    public Vector<OpenPositionInfo> getPositionsOpenCollection() {
        return positions_open_collection;
    }
}
