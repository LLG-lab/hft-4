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

import com.dukascopy.api.Instrument;
import com.dukascopy.api.system.ClientFactory;
import com.dukascopy.api.system.IClient;
import com.dukascopy.api.system.ISystemListener;
import java.nio.file.Files;
import java.nio.charset.Charset;
import java.nio.file.Paths;

import org.json.*;

import java.util.HashSet;
import java.util.Set;
import java.util.Vector; // XXX usunac potem
import java.io.*;

public class Main {

    private static String jnlpUrl = "";

    private static String userName = "";
    private static String password = "";

    private static IClient client;

    private static ProxyBridgeConfig config = null;

    private static int lightReconnects = 3;

    public static void main(String[] args) throws Exception {
/////////////////////////////////////////////////////////////////////////
///////////////// To jest do testów responsa z HFT (chyba działało) /////
//
//        String resp1 = "{\"status\":\"ack\"}";
//        String resp2 = "{\"status\":\"error\",\"message\":\"Here is the error message\"}";
//        String resp3 = "{\"status\":\"advice\",\"operations\":[{\"op\":\"close\",\"id\":\"0x0d0f87d\"},{\"op\":\"SHORT\",\"id\":\"DUPA777\",\"qty\":1}]}";
//
//        HftResponse response = new HftResponse(resp3);
//
//        if (response.isError()) {
//            System.out.println("Odpowiedź błędna: " + response.getErrorMessage());
//        } else {
//            System.out.println("Odpowiedź poprawna.");
//        }

//        Vector<String> do_zamk = response.getPositionsCloseCollection();
//        for (String x : do_zamk) {
//            System.out.println("Pozycja do zamknięcia: "+x);
//        }
//        Vector<HftResponse.OpenPositionInfo> do_otw = response.getPositionsOpenCollection();
//        for (HftResponse.OpenPositionInfo x : do_otw) {
//            System.out.println("Pozycja do otwarcia:");
//            System.out.println("\tTyp pozycji: " + (x.is_long ? "LONG" : "SHORT" ) );
//            System.out.println("\tID: " + x.id);
//            System.out.println("\tilosc: " + x.qty);
//        }
//        System.exit(0);
/////////////////////////////////////////////////////////////////////////
        try {
            config = new ProxyBridgeConfig(args);
        } catch (Exception e) {
            System.err.println("ERROR: " + e.getMessage());
            System.exit(1);
        }

        if (config.shouldHelp()) {
            System.out.println("hft2ducascopy - HFT ⇌ Dukascopy intermediary program");
            System.out.println("Copyright © 2017-2022 by LLG Ryszard Gradowski, All Rights Reserved");
            System.out.println("");
            System.out.println("Usage:");
            System.out.println("java -cp  hft-bridge-4.0.0.jar:* hft2ducascopy/Main [options]");
            System.out.println("");
            System.out.println("Available options:");
            System.out.println(config.getHelpMessage());

            System.exit(0);
        }

        jnlpUrl  = config.getUrl();
        userName = config.getLogin();
        password = config.getPassword();

        //
        // Get the instance of the IClient interface.
        //

        client = ClientFactory.getDefaultInstance();

        setSystemListener();

        System.out.println("Connecting...");

        try {
            tryToConnect();
        } catch (Exception e) {
            System.err.println("ERROR: " + e.getMessage());
        }

        if (! client.isConnected()) {
            System.err.println("ERROR: Failed to connect Dukascopy servers");
            System.exit(1);
        }

        System.out.println("Starting Proxy Bridge operations");

        client.startStrategy(new ProxyBridge(config));
        //now it's running
    }

    private static void setSystemListener() {
        //set the listener that will receive system events
        client.setSystemListener(new ISystemListener() {

            @Override
            public void onStart(long processId) {
                System.out.println("Strategy started: " + processId);
            }

            @Override
            public void onStop(long processId) {
                System.out.println("Strategy stopped: " + processId);
                if (client.getStartedStrategies().size() == 0) {
                    System.exit(0);
                }
            }

            @Override
            public void onConnect() {
                System.out.println("Connected");
                lightReconnects = 3;
            }

            @Override
            public void onDisconnect() {
                tryToReconnect();
            }
        });
    }

    private static void tryToConnect() throws Exception {
        //connect to the server using jnlp, user name and password
        client.connect(jnlpUrl, userName, password);

        //wait for it to connect
        int i = 10; //wait max ten seconds
        while (i > 0 && !client.isConnected()) {
            Thread.sleep(1000);
            i--;
        }
    }

    private static void tryToReconnect() {
        Runnable runnable = new Runnable() {
            @Override
            public void run() {
                while (! client.isConnected()) {
                    try {
                        if (lightReconnects > 0 && client.isReconnectAllowed()) {
                            client.reconnect();
                            --lightReconnects;
                        } else {
                            tryToConnect();
                        }
                        if(client.isConnected()) {
                            break;
                        }
                    } catch (Exception e) {
                        System.err.println("ERROR: " + e.getMessage());
                    }
                    sleep(60 * 1000);
                }
            }

            private void sleep(long millis) {
                try {
                    Thread.sleep(millis);
                } catch (InterruptedException e) {
                    System.err.println("ERROR: " + e.getMessage());
                }
            }
        };
        new Thread(runnable).start();
    }
}
