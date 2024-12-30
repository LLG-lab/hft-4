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

//import javafx.util.Pair;
import java.util.Scanner;
import java.util.HashSet;

import org.w3c.dom.*;
import org.xml.sax.SAXException;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.parsers.ParserConfigurationException;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;

public class ProxyBridgeConfig {
    private int port;
    private String host;
    private String config_file_name;
    private String sessid;
    private String url;
    private String login;
    private String password;
    private HashSet<String> instruments;
    private boolean should_help;

    ProxyBridgeConfig(String[] args) throws Exception {
        port = 8137;
        host = "127.0.0.1";
        config_file_name = "/etc/hft/hft-config.xml";
        sessid = "dukascopy-session";
        url = "UNDEFINED";
        login = "UNDEFINED";
        password = "UNDEFINED";
        instruments = new HashSet<String>();
        should_help = false;

        int i = 0;
        Pair<String, String> parameter;

        while (i < args.length) {
            parameter = parseArg(args[i]);

            if (parameter.getKey().equals("-h") || parameter.getKey().equals("--help")) {
                should_help = true;
            } else if (parameter.getKey().equals("--host")) {
                host = parameter.getValue();
            } else if (parameter.getKey().equals("--port")) {
                port = Integer.parseInt(parameter.getValue());
            } else if (parameter.getKey().equals("--config")) {
                config_file_name = parameter.getValue();
            } else if (parameter.getKey().equals("--sessid")) {
                sessid = parameter.getValue();
            } else {
                String error = "Invalid argument ‘" + parameter.getKey() + "’";
                throw new Exception(error);
            }

            i++;
        }

        //
        // Other configuration data to be parsed from the xml file.
        //

        parseXmlConfig();

        //
        // Basic validation.
        //

        if (url.equals("UNDEFINED")) {
            throw new Exception("Unspecified market account type in configuration ‘" + getConfigFileName() + "’");
        }

        if (login.equals("UNDEFINED")) {
            throw new Exception("Unspecified login to market account in configuration ‘" + getConfigFileName() + "’");
        }

        if (password.equals("UNDEFINED")) {
            throw new Exception("Unspecified password to account in configuration ‘" + getConfigFileName() + "’");
        }
    }

    private static Pair<String, String> parseArg(String arg) {
        String key = "";
        String value = "";

        Scanner scanner = new Scanner(arg);
        scanner.useDelimiter("=");
        int i = 0;

        while (scanner.hasNext()) {
            if (i == 0) {
                key = scanner.next();
            } else if (i == 1) {
                value = scanner.next();
            } else {
                value = value + "=" + scanner.next();
            }

            i++;
        }

        return new Pair<>(key, value);
    }

    private void parseXmlConfig() throws Exception {
        // Instantiate the Factory.
        DocumentBuilderFactory dbf = DocumentBuilderFactory.newInstance();

        try {
            // Parse XML file.
            DocumentBuilder db = dbf.newDocumentBuilder();

            Document doc = db.parse(new File(getConfigFileName()));
            doc.getDocumentElement().normalize();

            NodeList list = doc.getElementsByTagName("market");

            for (int temp = 0; temp < list.getLength(); temp++) {
                Node node = list.item(temp);

                if (node.getNodeType() == Node.ELEMENT_NODE) {
                    Element element = (Element) node;

                    if (element.getAttribute("bridge").equals("Dukascopy")
                            && element.getAttribute("sessid").equals(getSessId())) {
                        //
                        // We are now in our interested configuration node.
                        //

                        if (node.hasChildNodes()) {
                            obtainXmlConfigInfo(node.getChildNodes());

                            return;
                        }
                    }
                }
            }
        } catch (ParserConfigurationException | SAXException | IOException e) {
            throw new Exception(e.getMessage());
        }

        String error = "No configuration found for market ‘Dukascopy’, sessid ‘" +
                       getSessId() + "’ in config file ‘" +
                       getConfigFileName() + "’";

        throw new Exception(error);
    }

    private void obtainXmlConfigInfo(NodeList nodeList) throws Exception {
        for (int count = 0; count < nodeList.getLength(); count++) {

            Node node = nodeList.item(count);

            // Make sure it's element node.
            if (node.getNodeType() == Node.ELEMENT_NODE) {
                parseAuthNode(node);
                parseInstrumentNode(node);
            }
        }
    }

    private void parseAuthNode(Node node) throws Exception {
        if (node.getNodeName().equals("auth")) {

            //
            // Get account type from ‘account’ attribute.
            //

            if (node.hasAttributes()) {
                NamedNodeMap nodeMap = node.getAttributes();
                for (int i = 0; i < nodeMap.getLength(); i++) {
                    Node n = nodeMap.item(i);

                    if (n.getNodeName().equals("account")) {
                        if (n.getNodeValue().equals("demo")) {
                            url="http://platform.dukascopy.com/demo/jforex.jnlp";
                        } else if (n.getNodeValue().equals("live")) {
                            url="http://platform.dukascopy.com/live_3/jforex_3.jnlp";
                        } else {
                            String error = "Illegal value ‘" + n.getNodeValue() +
                                           "’ for attribute ‘account’ in xml file ‘" +
                                           getConfigFileName() + "’";

                            throw new Exception(error);
                        }
                    }
                }
            }

            //
            // Get login and password.
            //

            Element element = (Element) node;

            try {
                login = element.getElementsByTagName("login").item(0).getTextContent();
            } catch (NullPointerException e) {
                String error = "No login information found for market ‘Dukascopy’, sessid ‘" +
                       getSessId() + "’ in config file ‘" +
                       getConfigFileName() + "’";

                throw new Exception(error);
            }

            try {
                password = element.getElementsByTagName("password").item(0).getTextContent();
            } catch (NullPointerException e) {
                String error = "No password information found for market ‘Dukascopy’, sessid ‘" +
                       getSessId() + "’ in config file ‘" +
                       getConfigFileName() + "’";

                throw new Exception(error);
            }
        }
    }

    private void parseInstrumentNode(Node node) throws Exception {
        if (node.getNodeName().equals("instrument")) {

            //
            // Get particular instrument information
            // from ‘ticker’ attribute.
            //

            if (node.hasAttributes()) {
                NamedNodeMap nodeMap = node.getAttributes();
                for (int i = 0; i < nodeMap.getLength(); i++) {
                    Node n = nodeMap.item(i);

                    if (n.getNodeName().equals("ticker")) {
                        instruments.add(n.getNodeValue());

                        return;
                    }
                }
            }

            String error = "No instrument ticker information found for market ‘Dukascopy’, sessid ‘" +
                       getSessId() + "’ in config file ‘" +
                       getConfigFileName() + "’";

            throw new Exception(error);
        }
    }

    public int getPort() {
        return port;
    }

    public String getHost() {
        return host;
    }

    public String getConfigFileName() {
        return config_file_name;
    }

    public String getSessId() {
        return sessid;
    }

    public String getUrl() {
        return url;
    }

    public String getLogin() {
        return login;
    }

    public String getPassword() {
        return password;
    }

    public HashSet<String> getInstruments() {
        return instruments;
    }

    public boolean shouldHelp() {
        return should_help;
    }

    public String getHelpMessage() {
        return "-h|--help                           Show this help message\n"+
               "--port [=8137]                      Override connection port value\n"+
               "--host [=127.0.0.1]                 Override connection host\n"+
               "--config [=/etc/hft/hft-config.xml] Override default config file\n"+
               "--sessid [=dukascopy-session]       Setup session name\n"+
               "\n";
    }
}
