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

package hft2ducascopy;

import java.io.*;
import java.net.*;
import java.util.*;
import java.time.LocalDateTime;
import java.time.format.DateTimeFormatter;

import org.json.*;

import com.dukascopy.api.IEngine.OrderCommand;
import com.dukascopy.api.*;

public class ProxyBridge implements IStrategy {

    private IEngine  engine  = null;
    private IContext context = null;
    private IAccount account = null;

    private ProxyBridgeConfig config = null;
    private HftConnector hft = null;

    private Set<Instrument> instruments;
    private int chkSubscribeCounter;
    private int orderCounter;

    public ProxyBridge(ProxyBridgeConfig cfg) throws Exception {

        config = cfg;
        hft = new HftConnector(config);

        instruments = new HashSet<>();
        chkSubscribeCounter = 0;
        orderCounter = 0;

        System.out.println("ProxyBridge: initialize set of instruments based on configuration");

        for (String instr : config.getInstruments()) {
            Instrument i = Instrument.fromString(instr);

            if (i == null) {
                System.err.println("ProxyBridge: WARNING! Unrecognized instrument ‘"+instr+"’");
            } else {
                instruments.add(Instrument.fromString(instr));
                System.out.println("ProxyBridge: Instrument ‘"+instr+"’ added to collection");
            }
        }
    }

    private void checkDukascopySubscribed() {
        if (! context.getSubscribedInstruments().containsAll(instruments)
               && chkSubscribeCounter <= 0)  {
            System.out.println("WARNING! Some instruments still unsubscribed");

            context.setSubscribedInstruments(instruments);
            chkSubscribeCounter = 50;
        }

        chkSubscribeCounter--;
    }

// FIXME: To będzie raczej nie potrzebne.
    private void closeOrders(List<IOrder> orders) throws JFException
    {
        for (IOrder o: orders)
        {
            if (o.getState() == IOrder.State.FILLED || o.getState() == IOrder.State.OPENED)
            {
                o.close();
            }
        }
    }

    private void closePositionByLabel(String label) throws JFException {
        for (IOrder o : engine.getOrders()) {
            if (o.getState() == IOrder.State.FILLED && o.getLabel().equals(label) ) {
                o.close();
            }
        }
    }

    public void onStart(IContext context) throws JFException {
        System.out.println("Starting Proxy Bridge: onStart notify got called.");

        this.engine = context.getEngine();
        this.context = context;

        //
        // Here is an initial subsribe instruments to the Dukascopy
        //

        checkDukascopySubscribed();

        //
        // Synchronize opened positions with HFT.
        //

        for (IOrder o : engine.getOrders()) {
            if(o.getState() == IOrder.State.FILLED) {

                Instrument instr = o.getInstrument();

                if (instruments.contains(instr)) {
                    String instr_str = instr.toString();
                    boolean is_long  = o.isLong();
                    double  price    = o.getOpenPrice();
                    int     amount   = (int) (o.getRequestedAmount() * 1000000);
                    String  label    = o.getLabel(); // Chyba to będzie to o co nam chodzi
                    String  id       = o.getId();
                    String  comment  = o.getComment();
                    long    created  = o.getCreationTime();

                    System.out.println("----- Synchronizing Instrument [" + instr_str + "]-----------");
                    System.out.println("is long? [" + is_long + "]");
                    System.out.println("price [" + price + "]");
                    System.out.println("amount [" + amount + "]");
                    System.out.println("label [" + label + "]");
                    System.out.println("id [" + id + "]");
                    System.out.println("comment [" + comment + "]");

                    try {
                        hft.sync(instr_str, label, created, is_long, price, amount);
                    } catch (Exception e) {
                        System.err.println("WARNING! Failed to synchronize position to hft server: [" + e.getMessage() + ']');
                    }
                }
            }
        }

    }

    public void onStop() throws JFException
    {
        System.out.println("onStop notify got called.");
    }

    private void submitOrder(Instrument instrument, boolean isLong, String label, int qty) throws JFException
    {
        OrderCommand orderCmd;

        if (isLong)
        {
            orderCmd = OrderCommand.BUY;
        }
        else
        {
            orderCmd = OrderCommand.SELL;
        }

        double amount = (double) (qty) / 1000000;

        engine.submitOrder(label, instrument, orderCmd, amount);
    }

    public void onTick(Instrument instrument, ITick tick) throws JFException
    {
        if (! instruments.contains(instrument)) {
            return;
        }

        String instr_str = instrument.toString();
        double ask = tick.getAsk();
        double bid = tick.getBid();
        double equity = account.getEquity();
        double free_margin = equity - account.getUsedMargin();

        HftResponse resp;

        try {
            resp = hft.tick(instr_str, ask, bid, equity, free_margin);
        } catch (Exception e) {
            System.err.println("WARNING! Method tick failed: [" + e.getMessage() + ']');
            return;
        }

        if (resp.isError()) {
            System.err.println("ERROR HFT! ‘" + resp.getErrorMessage() + "’");
            return;
        }

        //
        // Try to close positions, if any.
        //

        Vector<String> to_close = resp.getPositionsCloseCollection();
        for (String x : to_close) {
            closePositionByLabel(x);
        }

        //
        // Open new positions, if any.
        //

        Vector<HftResponse.OpenPositionInfo> to_open = resp.getPositionsOpenCollection();
        for (HftResponse.OpenPositionInfo x : to_open) {
              submitOrder(instrument, x.is_long, x.id, x.qty);
//            System.out.println("Pozycja do otwarcia:");
//            System.out.println("\tTyp pozycji: " + (x.is_long ? "LONG" : "SHORT" ) );
//            System.out.println("\tID: " + x.id);
//            System.out.println("\tilosc: " + x.qty);
        }
    }

    public void onBar(Instrument instrument, Period period, IBar askBar, IBar bidBar)
    {
        /* Nothing to do */
    }

    public void onMessage(IMessage message) throws JFException
    {
        switch (message.getType())
        {
/* Sytuacje spoko */
            case ORDER_SUBMIT_OK: // olać
            {
                break;
            }
            case ORDER_FILL_OK:
            {
                String instr_str = message.getOrder().getInstrument().toString();
                String id = message.getOrder().getLabel();
                double price = message.getOrder().getOpenPrice();

                try {
                    hft.open_notify(instr_str, id, true, price);
                } catch (Exception e) {
                    System.err.println("WARNING! Method open_notify failed: [" + e.getMessage() + ']');
                }

                break;
            }
            case ORDER_CLOSE_OK:
            {
                String instr_str = message.getOrder().getInstrument().toString();
                String id = message.getOrder().getLabel();
                double price = message.getOrder().getClosePrice();

                try {
                    hft.close_notify(instr_str, id, true, price);
                } catch (Exception e) {
                    System.err.println("WARNING! Method close_notify failed: [" + e.getMessage() + ']');
                }

                break;
            }
/* Zjebane sytuacje */
            case ORDER_CLOSE_REJECTED:
            {
                String instr_str = message.getOrder().getInstrument().toString();
                String id = message.getOrder().getLabel();

                try {
                    hft.close_notify(instr_str, id, false, 0.00);
                } catch (Exception e) {
                    System.err.println("WARNING! Method close_notify failed: [" + e.getMessage() + ']');
                }

                break;
            }
            case ORDER_SUBMIT_REJECTED:
            {
                String instr_str = message.getOrder().getInstrument().toString();
                String id = message.getOrder().getLabel();

                try {
                    hft.open_notify(instr_str, id, false, 0.00);
                } catch (Exception e) {
                    System.err.println("WARNING! Method open_notify failed: [" + e.getMessage() + ']');
                }

                break;
            }
            case ORDER_FILL_REJECTED:
            {
                String instr_str = message.getOrder().getInstrument().toString();
                String id = message.getOrder().getLabel();

                try {
                    hft.open_notify(instr_str, id, false, 0.00);
                } catch (Exception e) {
                    System.err.println("WARNING! Method open_notify failed: [" + e.getMessage() + ']');
                }

                break;
            }
        }
    }

    public void onAccount(IAccount acc) throws JFException
    {
        System.out.println("onAccount notify got called.");

        this.account = acc;
    }
}
