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

#include <easylogging++.h>

#include <market_session.hpp>

#define hft2ctrader_log(__X__) \
    CLOG(__X__, "market")

//
// In production should be undefined.
//

#define MARKET_SESSION_TEST

#ifdef MARKET_SESSION_TEST
namespace {

void display_execution_event(const ProtoOAExecutionEvent &event);
void display_protooaposition(const ProtoOAPosition &pos, const std::string &prepend);
void display_tradedata(const ProtoOATradeData &tradedata, const std::string &prepend);
void display_protooaorder(const ProtoOAOrder &order, const std::string &prepend);
void display_protooadeal(const ProtoOADeal &deal, const std::string &prepend);
void display_closepositiondetail(const ProtoOAClosePositionDetail &cpd, const std::string &prepend);

} // namespace
#endif /* MARKET_SESSION_TEST */

market_session::market_session(ctrader_ssl_connection &connection, const hft2ctrader_bridge_config &config)
    : ctrader_session(connection, config), config_(config)
{
    el::Loggers::getLogger("market", true);
}

void market_session::on_init(void)
{
    hft2ctrader_log(TRACE) << "on_init – got called.";

    subscribe_instruments_ex({ "EURUSD", "GBPUSD"});
}

void market_session::on_tick(const tick_type &tick)
{
    static bool first_time = true;

    if (first_time)
    {
        first_time = false;

        hft2ctrader_log(TRACE) << "on_tick. instrument: ‘" << tick.instrument
                               << "’, ask: ‘" << tick.ask
                               << "’, bid: ‘" << tick.bid
                               << "’, timestamp: ‘" << tick.timestamp
                               << "’.";

      //  create_market_order_ex("wdupie", tick.instrument, position_type::SHORT_POSITION, 100000);
    }
}

void market_session::on_order_execution_event(const ProtoOAExecutionEvent &event)
{
    #ifdef MARKET_SESSION_TEST
    display_execution_event(event);
    #endif

}

//
// Testing stuff.
//

#ifdef MARKET_SESSION_TEST
namespace {

void display_execution_event(const ProtoOAExecutionEvent &event)
{
    hft2ctrader_log(TRACE) << "Execution event:";

    switch (event.executiontype())
    {
        case ORDER_ACCEPTED:
             hft2ctrader_log(TRACE) << "\texecutionType: ORDER_ACCEPTED";
             break;
        case ORDER_FILLED:
             hft2ctrader_log(TRACE) << "\texecutionType: ORDER_FILLED";
             break;
        case ORDER_REPLACED:
             hft2ctrader_log(TRACE) << "\texecutionType: ORDER_REPLACED";
             break;
        case ORDER_CANCELLED:
             hft2ctrader_log(TRACE) << "\texecutionType: ORDER_CANCELLED";
             break;
        case ORDER_EXPIRED:
             hft2ctrader_log(TRACE) << "\texecutionType: ORDER_EXPIRED";
             break;
        case ORDER_REJECTED:
             hft2ctrader_log(TRACE) << "\texecutionType: ORDER_REJECTED";
             break;
        case ORDER_CANCEL_REJECTED:
             hft2ctrader_log(TRACE) << "\texecutionType: ORDER_CANCEL_REJECTED";
             break;
        case ORDER_PARTIAL_FILL:
             hft2ctrader_log(TRACE) << "\texecutionType: ORDER_PARTIAL_FILL";
             break;
    }

    if (event.has_errorcode())
    {
        //
        // The name of the ProtoErrorCode or the other
        // custom ErrorCodes (e.g. ProtoCHErrorCode).
        //

        hft2ctrader_log(TRACE) << "\terrorCode: " << event.errorcode();
    }

    if (event.has_position())
    {
        hft2ctrader_log(TRACE) << "\tposition:";
        display_protooaposition(event.position(), "\t\t");
    }

    if (event.has_order())
    {
        hft2ctrader_log(TRACE) << "\torder:";
        display_protooaorder(event.order(), "\t\t");
    }

    if (event.has_deal())
    {
        hft2ctrader_log(TRACE) << "\tdeal:";
        display_protooadeal(event.deal(), "\t\t");
    }

    //
    // bonusDepositWithdraw and depositWithdraw
    // was deliberately omitted.
    //

    if (event.has_isserverevent())
    {
        //
        // If TRUE then the event generated by the
        // server logic instead of the trader's
        // request. (e.g. stop-out).
        //

        hft2ctrader_log(TRACE) << "\tisServerEvent: "
                               << (event.isserverevent() ? "YES" : "NO");
    }
}

void display_protooaposition(const ProtoOAPosition &pos, const std::string &prepend)
{
/*
  Field                  Type                        Label         Description
-------------------------------------------------------------------------------------------------------------------
positionid	            int64	                    required	The unique ID of the position. Note: trader might have
                                                                two positions with the same id if positions are taken
                                                                from accounts from different brokers.
                                                                ---
tradedata	            ProtoOATradeData	        required	Position details. See ProtoOATradeData for details.
                                                                --- 
positiondtatus	        ProtoOAPositionStatus	    required	Current status of the position.
                                                                ---
swap	                int64	                    required	Total amount of charged swap on open position.
                                                                ---
price	                double	                    optional	VWAP price of the position based on all executions
                                                                (orders) linked to the position.
                                                                ---
stoploss	            double	                    optional	Current stop loss price.
                                                                --- 
takeprofit	            double	                    optional	Current take profit price.
                                                                ---
utclastupdatetimestamp	int64	                    optional	Time of the last change of the position, including
                                                                amend SL/TP of the position, execution of related
                                                                order, cancel or related order, etc.
                                                                ---
commission	            int64	                    optional	Current unrealized commission related to the position.
                                                                ---
marginrate	            double	                    optional	Rate for used margin computation. Represented as Base/Deposit.
                                                                ---
mirroringcommission	    int64	                    optional	Amount of unrealized commission related to
                                                                following of strategy provider.
                                                                ---
guaranteedstoploss	    bool	                    optional	If TRUE then position's stop loss is guaranteedStopLoss.
                                                                ---
usedmargin	            uint64	                    optional	Amount of margin used for the position in deposit currency.
                                                                ---
stoplosstriggermethod	ProtoOAOrderTriggerMethod	optional	Stop trigger method for SL/TP of the position. Default: TRADE
                                                                ---
moneydigits	            uint32	                    optional	Specifies the exponent of the monetary values.
                                                                E.g. moneyDigits = 8 must be interpret as business
                                                                value multiplied by 10^8, then real balance
                                                                would be 10053099944 / 10^8 = 100.53099944.
                                                                Affects swap, commission, mirroringCommission, usedMargin.
                                                                ---
trailingstoploss	    bool	                    optional	If TRUE then the Trailing Stop Loss is applied.
*/
    hft2ctrader_log(TRACE) << prepend << "positionId: " << pos.positionid();
    hft2ctrader_log(TRACE) << prepend << "tradeData:";

    display_tradedata(pos.tradedata(), prepend + "\t");

    switch (pos.positionstatus())
    {
        case POSITION_STATUS_OPEN:
            hft2ctrader_log(TRACE) << prepend << "positionStatus: POSITION_STATUS_OPEN";
            break;
        case POSITION_STATUS_CLOSED:
            hft2ctrader_log(TRACE) << prepend << "positionStatus: POSITION_STATUS_CLOSED";
            break;
        case POSITION_STATUS_CREATED: // Empty position is created for pending order.
            hft2ctrader_log(TRACE) << prepend << "positionStatus: POSITION_STATUS_CREATED";
            break;
        case POSITION_STATUS_ERROR:
            hft2ctrader_log(TRACE) << prepend << "positionStatus: POSITION_STATUS_ERROR";
            break;
    }

    hft2ctrader_log(TRACE) << prepend << "swap: " << pos.swap();

    if (pos.has_price())
    {
        hft2ctrader_log(TRACE) << prepend << "price: " << pos.price();
    }

    if (pos.has_stoploss())
    {
        hft2ctrader_log(TRACE) << prepend << "stopLoss: " << pos.stoploss();
    }

    if (pos.has_takeprofit())
    {
        hft2ctrader_log(TRACE) << prepend << "takeProfit: " << pos.takeprofit();
    }

    if (pos.has_utclastupdatetimestamp())
    {
        hft2ctrader_log(TRACE) << prepend << "utcLastUpdateTimestamp: "
                               << pos.utclastupdatetimestamp();
    }

    if (pos.has_commission())
    {
        hft2ctrader_log(TRACE) << prepend << "commission: " << pos.commission();
    }

    if (pos.has_marginrate())
    {
        hft2ctrader_log(TRACE) << prepend << "marginRate: " << pos.marginrate();
    }

    if (pos.has_mirroringcommission())
    {
        hft2ctrader_log(TRACE) << prepend << "mirroringCommission: "
                               << pos.mirroringcommission();
    }

    if (pos.has_guaranteedstoploss())
    {
        hft2ctrader_log(TRACE) << prepend << "guaranteedStopLoss: "
                               << (pos.guaranteedstoploss() ? "yes" : "no");
    }

    if (pos.has_usedmargin())
    {
        hft2ctrader_log(TRACE) << prepend << "usedMargin: " << pos.usedmargin();
    }

    if (pos.has_stoplosstriggermethod())
    {
        switch (pos.stoplosstriggermethod())
        {
            case TRADE:
                // Stop Order: buy is triggered by ask, sell by bid;
                // Stop Loss Order: for buy position is triggered
                // by bid and for sell position by ask.

                hft2ctrader_log(TRACE) << prepend
                                       << "stopLossTriggerMethod: TRADE";

                break;
            case OPPOSITE:
                // Stop Order: buy is triggered by bid, sell by ask;
                // Stop Loss Order: for buy position is triggered
                // by ask and for sell position by bid.

                hft2ctrader_log(TRACE) << prepend
                                       << "stopLossTriggerMethod: OPPOSITE";

                break;
            case DOUBLE_TRADE:
                // The same as TRADE, but trigger is checked
                // after the second consecutive tick.

                hft2ctrader_log(TRACE) << prepend
                                       << "stopLossTriggerMethod: DOUBLE_TRADE";

                break;
            case DOUBLE_OPPOSITE:
                // The same as OPPOSITE, but trigger is checked
                // after the second consecutive tick.

                hft2ctrader_log(TRACE) << prepend
                                       << "stopLossTriggerMethod: DOUBLE_OPPOSITE";

                break;
        }
    }

    if (pos.has_moneydigits())
    {
        hft2ctrader_log(TRACE) << prepend << "moneyDigits: " << pos.moneydigits();
    }

    if (pos.has_trailingstoploss())
    {
        hft2ctrader_log(TRACE) << prepend << "trailingStopLoss: "
                               << pos.trailingstoploss();
    }
}

void display_tradedata(const ProtoOATradeData &tradedata, const std::string &prepend)
{
/*
   Field               Type              Label       Description
------------------------------------------------------------------------
symbolid	          int64             required   The unique identifier of the symbol in specific server
                                                   environment within cTrader platform. Different brokers
                                                   might have different IDs.
                                                   ---
volume	              int64             required   Volume in cents.
                                                   ---
tradeside	          ProtoOATradeSide  required   Buy, Sell.
                                                   ---
opentimestamp         int64             optional   Time when position was opened or order was created.
                                                   ---
label                 string            optional   Text label specified during order request.
                                                   ---
guaranteedstoploss	  bool              optional   If TRUE then position/order stop loss is guaranteedStopLoss.
                                                   ---
comment               string            optional   User-specified comment.

*/
    hft2ctrader_log(TRACE) << prepend << "symbolId: " << tradedata.symbolid();
    hft2ctrader_log(TRACE) << prepend << "volume: " << tradedata.volume();

    switch (tradedata.tradeside())
    {
        case BUY:
            hft2ctrader_log(TRACE) << prepend << "tradeSide: BUY";
            break;
        case SELL:
            hft2ctrader_log(TRACE) << prepend << "tradeSide: SELL";
            break;
    }

    if (tradedata.has_opentimestamp())
    {
        hft2ctrader_log(TRACE) << prepend << "openTimestamp: "
                               << tradedata.opentimestamp();
    }

    if (tradedata.has_label())
    {
        hft2ctrader_log(TRACE) << prepend << "label: " << tradedata.label();
    }

    if (tradedata.has_guaranteedstoploss())
    {
        hft2ctrader_log(TRACE) << prepend << "guaranteedStopLoss: "
                               << (tradedata.guaranteedstoploss() ? "YES" : "NO");
    }

    if (tradedata.has_comment())
    {
        hft2ctrader_log(TRACE) << prepend << "comment: ‘"
                               << tradedata.comment() << "’";
    }
}

void display_protooaorder(const ProtoOAOrder &order, const std::string &prepend)
{
/*
--------------------------------------------------------------------------------------
orderid                int64                        required    The unique ID of the order. Note: trader might have
                                                                two orders with the same id if orders are taken from
                                                                accounts from different brokers.
                                                                ---
tradedata              ProtoOATradeData             required    Detailed trader data.
                                                                ---
orderType              ProtoOAOrderType             required    Order type.
                                                                ---
orderStatus            ProtoOAOrderStatus           required    Order status.
                                                                ---
expirationTimestamp    int64                        optional    If the order has time in force GTD then expiration is specified.
                                                                ---
executionPrice         double                       optional    Price at which an order was executed. For order with FILLED status.
                                                                ---
executedVolume         int64                        optional    Part of the volume that was filled.
                                                                ---
utcLastUpdateTimestamp int64                        optional    Timestamp of the last update of the order.
                                                                ---
baseSlippagePrice      double                       optional    Used for Market Range order with combination of slippageInPoints
                                                                to specify price range were order can be executed.
                                                                ---
slippageInPoints       int64                        optional    Used for Market Range and STOP_LIMIT orders to to specify price
                                                                range were order can be executed.
                                                                ---
closingOrder           bool                         optional    If TRUE then the order is closing part of whole position.
                                                                Must have specified positionId.
                                                                ---
limitPrice             double                       optional    Valid only for LIMIT orders.
                                                                ---
stopPrice              double                       optional    Valid only for STOP and STOP_LIMIT orders.
                                                                ---
stopLoss               double                       optional    Absolute stopLoss price.
                                                                ---
takeProfit             double                       optional    Absolute takeProfit price.
                                                                ---
clientOrderId          string                       optional    Optional ClientOrderId. Max Length = 50 chars.
                                                                ---
timeInForce            ProtoOATimeInForce           optional    Order's time in force. Depends on order type.
                                                                Default: IMMEDIATE_OR_CANCEL
                                                                ---
positionId             int64                        optional    ID of the position linked to the order (e.g. closing order,
                                                                order that increase volume of a specific position, etc.).
                                                                ---
relativeStopLoss       int64                        optional    Relative stopLoss that can be specified instead of absolute
                                                                as one. Specified in 1/100_000 of unit of a price.
                                                                For BUY stopLoss = entryPrice - relativeStopLoss,
                                                                for SELL stopLoss = entryPrice + relativeStopLoss.
                                                                ---
relativeTakeProfit     int64                        optional    Relative takeProfit that can be specified instead of absolute one.
                                                                Specified in 1/100_000 of unit of a price.
                                                                ForBUY takeProfit = entryPrice + relativeTakeProfit,
                                                                for SELL takeProfit = entryPrice - relativeTakeProfit.
                                                                ---
isStopOut              bool                         optional    If TRUE then order was stopped out from server side.
                                                                ---
trailingStopLoss       bool                         optional    If TRUE then order is trailingStopLoss. Valid for
                                                                STOP_LOSS_TAKE_PROFIT order.
                                                                --- 
stopTriggerMethod      ProtoOAOrderTriggerMethod    optional    Trigger method for the order. Valid only for STOP
                                                                and STOP_LIMIT orders. Default: TRADE
*/

    hft2ctrader_log(TRACE) << prepend << "orderId: " << order.orderid();

    hft2ctrader_log(TRACE) << prepend << "tradeData:";
    display_tradedata(order.tradedata(), prepend + "\t");

    switch (order.ordertype())
    {
        case MARKET:
            hft2ctrader_log(TRACE) << prepend << "orderType: MARKET";
            break;

        case LIMIT:
            hft2ctrader_log(TRACE) << prepend << "orderType: LIMIT";
            break;

        case STOP:
            hft2ctrader_log(TRACE) << prepend << "orderType: STOP";
            break;

        case STOP_LOSS_TAKE_PROFIT:
            hft2ctrader_log(TRACE) << prepend << "orderType: STOP_LOSS_TAKE_PROFIT";
            break;

        case MARKET_RANGE:
            hft2ctrader_log(TRACE) << prepend << "orderType: MARKET_RANGE";
            break;

        case STOP_LIMIT:
            hft2ctrader_log(TRACE) << prepend << "orderType: STOP_LIMIT";
            break;
    }

    switch (order.orderstatus())
    {
        case ORDER_STATUS_ACCEPTED:
	        // Order request validated and accepted for execution.
            hft2ctrader_log(TRACE) << prepend << "orderStatus: ORDER_STATUS_ACCEPTED";
            break;

        case ORDER_STATUS_FILLED:
            // Order is fully filled.
            hft2ctrader_log(TRACE) << prepend << "orderStatus: ORDER_STATUS_FILLED";
            break;

        case ORDER_STATUS_REJECTED:
            // Order is rejected due to validation.
            hft2ctrader_log(TRACE) << prepend << "orderStatus: ORDER_STATUS_REJECTED";
            break;

        case ORDER_STATUS_EXPIRED:
            // Order expired. Might be valid for orders
            // with partially filled volume that
            // were expired on LP.
            hft2ctrader_log(TRACE) << prepend << "orderStatus: ORDER_STATUS_EXPIRED";
            break;

        case ORDER_STATUS_CANCELLED:
            // Order is cancelled. Might be valid for
            // orders with partially filled volume that
            // were cancelled by LP.
            hft2ctrader_log(TRACE) << prepend << "orderStatus: ORDER_STATUS_CANCELLED";
            break;
    }

    if (order.has_expirationtimestamp())
    {
        hft2ctrader_log(TRACE) << prepend << "expirationTimestamp: " << order.expirationtimestamp();
    }

    if (order.has_executionprice())
    {
        hft2ctrader_log(TRACE) << prepend << "executionPrice: " << order.executionprice();
    }

    if (order.has_executedvolume())
    {
        hft2ctrader_log(TRACE) << prepend << "executedVolume: " << order.executedvolume();
    }

    if (order.has_utclastupdatetimestamp())
    {
        hft2ctrader_log(TRACE) << prepend << "utcLastUpdateTimestamp: " << order.utclastupdatetimestamp();
    }

    if (order.has_baseslippageprice())
    {
        hft2ctrader_log(TRACE) << prepend << "baseSlippagePrice: " << order.baseslippageprice();
    }

    if (order.has_slippageinpoints())
    {
        hft2ctrader_log(TRACE) << prepend << "slippageInPoints: " << order.slippageinpoints();
    }

    if (order.has_closingorder())
    {
        hft2ctrader_log(TRACE) << prepend << "closingOrder: " << (order.closingorder() ? "YES" : "NO");
    }

    if (order.has_limitprice())
    {
        hft2ctrader_log(TRACE) << prepend << "limitPrice: " << order.limitprice();
    }

    if (order.has_stopprice())
    {
        hft2ctrader_log(TRACE) << prepend << "stopPrice: " << order.stopprice();
    }

    if (order.has_stoploss())
    {
        hft2ctrader_log(TRACE) << prepend << "stopLoss: " << order.stoploss();
    }

    if (order.has_takeprofit())
    {
        hft2ctrader_log(TRACE) << prepend << "takeProfit: " << order.takeprofit();
    }

    if (order.has_clientorderid())
    {
        hft2ctrader_log(TRACE) << prepend << "clientOrderId: ‘" << order.clientorderid() << "’";
    }

    if (order.has_timeinforce())
    {
        switch (order.timeinforce())
        {
            case GOOD_TILL_DATE:
                hft2ctrader_log(TRACE) << prepend << "timeInForce: GOOD_TILL_DATE";
                break;

            case GOOD_TILL_CANCEL:
                hft2ctrader_log(TRACE) << prepend << "timeInForce: GOOD_TILL_CANCEL";
                break;

            case IMMEDIATE_OR_CANCEL:
                hft2ctrader_log(TRACE) << prepend << "timeInForce: IMMEDIATE_OR_CANCEL";
                break;

            case FILL_OR_KILL:
                hft2ctrader_log(TRACE) << prepend << "timeInForce: FILL_OR_KILL";
                break;

            case MARKET_ON_OPEN:
                hft2ctrader_log(TRACE) << prepend << "timeInForce: MARKET_ON_OPEN";
                break;
        }
    }

    if (order.has_positionid())
    {
        hft2ctrader_log(TRACE) << prepend << "positionId: " << order.positionid();
    }

    if (order.has_relativestoploss())
    {
        hft2ctrader_log(TRACE) << prepend << "relativeStopLoss: " << order.relativestoploss();
    }

    if (order.has_relativetakeprofit())
    {
        hft2ctrader_log(TRACE) << prepend << "relativeTakeProfit: " << order.relativetakeprofit();
    }

    if (order.has_isstopout())
    {
        hft2ctrader_log(TRACE) << prepend << "isStopOut: " << (order.isstopout() ? "YES" : "NO");
    }

    if (order.has_trailingstoploss())
    {
        hft2ctrader_log(TRACE) << prepend << "trailingStopLoss: " << (order.trailingstoploss() ? "YES" : "NO");
    }

    if (order.has_stoptriggermethod())
    {
        switch (order.stoptriggermethod())
        {
            case TRADE:
                // Stop Order: buy is triggered by ask, sell by bid;
                // Stop Loss Order: for buy position is triggered by bid and for sell position by ask.

                hft2ctrader_log(TRACE) << prepend << "stopTriggerMethod: TRADE";

                break;

            case OPPOSITE:
                // Stop Order: buy is triggered by bid, sell by ask;
                // Stop Loss Order: for buy position is triggered by ask and for sell position by bid.

                hft2ctrader_log(TRACE) << prepend << "stopTriggerMethod: OPPOSITE";

                break;

            case DOUBLE_TRADE:
                // The same as TRADE, but trigger is checked after the second consecutive tick.

                hft2ctrader_log(TRACE) << prepend << "stopTriggerMethod: DOUBLE_TRADE";

                break;

            case DOUBLE_OPPOSITE:
                // The same as OPPOSITE, but trigger is checked after the second consecutive tick.

                hft2ctrader_log(TRACE) << prepend << "stopTriggerMethod: DOUBLE_OPPOSITE";

                break;
        }
    }
}

void display_protooadeal(const ProtoOADeal &deal, const std::string &prepend)
{
/*
   Field                   Type                           Label        Description
--------------------------------------------------------------------------------------------------
dealId                     int64                         required    The unique ID of the execution deal.
                                                                     ---
orderId                    int64                         required    Source order of the deal.
                                                                     ---
positionId                 int64                         required    Source position of the deal.
                                                                     ---
volume                     int64                         required    Volume sent for execution, in cents.
                                                                     ---
filledVolume               int64                         required    Filled volume, in cents.
                                                                     ---
symbolId                   int64                         required    The unique identifier of the symbol in specific server
                                                                     environment within cTrader platform. Different servers
                                                                     have different IDs.
                                                                     ---
createTimestamp            int64                         required    Time when the deal was sent for execution.
                                                                     ---
executionTimestamp         int64                         required    Time when the deal was executed.
                                                                     ---
utcLastUpdateTimestamp     int64                         optional    Timestamp when the deal was created, executed or rejected.
                                                                     ---
executionPrice             double                        optional    Execution price.
                                                                     ---
tradeSide                  ProtoOATradeSide              required    Buy/Sell.
                                                                     ---
dealStatus                 ProtoOADealStatus             required    Status of the deal.
                                                                     ---
marginRate                 double	                     optional    Rate for used margin computation. Represented as Base/Deposit.
                                                                     ---
commission                 int64	                     optional    Amount of trading commission associated with the deal.
                                                                     ---
baseToUsdConversionRate    double	                     optional    Base to USD conversion rate on the time of deal execution.
                                                                     ---
closePositionDetail        ProtoOAClosePositionDetail    optional    Closing position detail. Valid only for closing deal.
                                                                     ---
moneyDigits                uint32	                     optional    Specifies the exponent of the monetary values. E.g. moneyDigits = 8
                                                                     must be interpret as business value multiplied by 10^8, then real
                                                                     balance would be 10053099944 / 10^8 = 100.53099944. Affects commission.
*/
    hft2ctrader_log(TRACE) << prepend << "dealId: " << deal.dealid();
    hft2ctrader_log(TRACE) << prepend << "orderId: " << deal.orderid();
    hft2ctrader_log(TRACE) << prepend << "volume: " << deal.volume();
    hft2ctrader_log(TRACE) << prepend << "filledVolume: " << deal.filledvolume();
    hft2ctrader_log(TRACE) << prepend << "symbolId: " << deal.symbolid();
    hft2ctrader_log(TRACE) << prepend << "createTimestamp: " << deal.createtimestamp();
    hft2ctrader_log(TRACE) << prepend << "executionTimestamp: " << deal.executiontimestamp();

    if (deal.has_utclastupdatetimestamp())
    {
        hft2ctrader_log(TRACE) << prepend << "utcLastUpdateTimestamp: " << deal.utclastupdatetimestamp();
    }

    if (deal.has_executionprice())
    {
        hft2ctrader_log(TRACE) << prepend << "executionPrice: " << deal.executionprice();
    }

    switch (deal.tradeside())
    {
        case BUY:
            hft2ctrader_log(TRACE) << prepend << "tradeSide: BUY";
            break;

        case SELL:
            hft2ctrader_log(TRACE) << prepend << "tradeSide: SELL";
            break;
    }

    switch (deal.dealstatus())
    {
        case FILLED:
            // Deal filled.
            hft2ctrader_log(TRACE) << prepend << "dealStatus: FILLED";
            break;

        case PARTIALLY_FILLED:
            // Deal is partially filled.
            hft2ctrader_log(TRACE) << prepend << "dealStatus: PARTIALLY_FILLED";
            break;

        case REJECTED:
            // Deal is correct but was rejected by liquidity provider (e.g. no liquidity).
            hft2ctrader_log(TRACE) << prepend << "dealStatus: REJECTED";
            break;

        case INTERNALLY_REJECTED:
            // Deal rejected by server (e.g. no price quotes).
            hft2ctrader_log(TRACE) << prepend << "dealStatus: INTERNALLY_REJECTED";
            break;

        case ERROR:
            // Deal is rejected by LP due to error (e.g. symbol is unknown).
            hft2ctrader_log(TRACE) << prepend << "dealStatus: ERROR";
            break;

        case MISSED:
            // Liquidity provider did not sent response on the deal during specified execution time period.
            hft2ctrader_log(TRACE) << prepend << "dealStatus: MISSED";
            break;
    }

    if (deal.has_marginrate())
    {
        hft2ctrader_log(TRACE) << prepend << "marginRate: " << deal.marginrate();
    }

    if (deal.has_commission())
    {
        hft2ctrader_log(TRACE) << prepend << "commission: " << deal.commission();
    }

    if (deal.has_basetousdconversionrate())
    {
        hft2ctrader_log(TRACE) << prepend << "baseToUsdConversionRate: " << deal.basetousdconversionrate();
    }

    if (deal.has_closepositiondetail())
    {
        hft2ctrader_log(TRACE) << prepend << "closePositionDetail:";

        display_closepositiondetail(deal.closepositiondetail(), prepend + "\t");
    }

    if (deal.has_moneydigits())
    {
        hft2ctrader_log(TRACE) << prepend << "moneyDigits: " << deal.moneydigits();
    }
}

void display_closepositiondetail(const ProtoOAClosePositionDetail &cpd, const std::string &prepend)
{
/*
Field                            Type      Label         Description
------------------------------------------------------------------------------
entryPrice                      double    required    Position price at the moment of filling the closing order.
                                                      ---
grossProfit                     int64     required    Amount of realized gross profit after closing deal execution.
                                                      ---
swap                            int64     required    Amount of realized swap related to closed volume.
                                                      ---
commission                      int64     required    Amount of realized commission related to closed volume.
                                                      ---
balance                         int64     required    Account balance after closing deal execution.
                                                      ---
quoteToDepositConversionRate    double    optional    Quote/Deposit currency conversion rate on the time
                                                      of closing deal execution.
                                                      ---
closedVolume                    int64     optional    Closed volume in cents.
                                                      ---
balanceVersion                  int64     optional    Balance version of the account related to closing deal operation.
                                                      ---
moneyDigits                     uint32    optional    Specifies the exponent of the monetary values. E.g. moneyDigits = 8
                                                      must be interpret as business value multiplied by 10^8, then real
                                                      balance would be 10053099944 / 10^8 = 100.53099944. Affects
                                                      grossProfit, swap, commission, balance, pnlConversionFee.
                                                      ---
pnlConversionFee                int64     optional    Fee for conversion applied to the Deal in account's ccy when trader
                                                      symbol's quote asset id <> ProtoOATrader.depositAssetId.
*/

    hft2ctrader_log(TRACE) << prepend << "entryPrice: " << cpd.entryprice();
    hft2ctrader_log(TRACE) << prepend << "grossProfit: " << cpd.grossprofit();
    hft2ctrader_log(TRACE) << prepend << "swap: " << cpd.swap();
    hft2ctrader_log(TRACE) << prepend << "commission: " << cpd.commission();
    hft2ctrader_log(TRACE) << prepend << "balance: " << cpd.balance();

    if (cpd.has_quotetodepositconversionrate())
    {
        hft2ctrader_log(TRACE) << prepend << "quoteToDepositConversionRate: "
                               << cpd.quotetodepositconversionrate();
    }

    if (cpd.has_closedvolume())
    {
        hft2ctrader_log(TRACE) << prepend << "closedVolume: " << cpd.closedvolume();
    }

    if (cpd.has_balanceversion())
    {
        hft2ctrader_log(TRACE) << prepend << "balanceVersion: " << cpd.balanceversion();
    }

    if (cpd.has_moneydigits())
    {
        hft2ctrader_log(TRACE) << prepend << "moneyDigits: " << cpd.moneydigits();
    }

    if (cpd.has_pnlconversionfee())
    {
        hft2ctrader_log(TRACE) << prepend << "pnlConversionFee: " << cpd.pnlconversionfee();
    }
}

} // namespace
#endif /* MARKET_SESSION_TEST */
