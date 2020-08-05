//
//  HTML PAGE
//

const char PAGE_APPSettings[] PROGMEM =  R"=====(
<meta name="viewport" content="width=device-width, initial-scale=1" />
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<a href="/"  class="btn btn--s"><</a>&nbsp;&nbsp;<strong>Bike Lean & Brake Settings</strong>
<hr>
<form action="" method="post">
<table border="0"  cellspacing="0" cellpadding="3" >
<tr>
	<td align="right">Break Threshold </td>
	<td><input type="text" id="brakeThrs" name="brakeThrs" value=""></td>
</tr>
<tr>
	<td align="right">ACC Threshold </td>
	<td><input type="text" id="accelThrs" name="accelThrs" value=""></td>
</tr>
<tr>
	<td align="right">Break Emergency</td>
	<td><input type="text" id="brakeEmg" name="brakeEmg" value=""></td>
</tr>
<tr>
	<td align="right">ACC Emergency</td>
	<td><input type="text" id="accelEmg" name="accelEmg" value=""></td>
</tr>
<tr>
	<td align="right">Breaking Axis (1=X, 2=Y, 0=Z)</td>
	<td><input type="text" id="accAxis" name="accAxis" value=""></td>
</tr>
<tr>
<td align="right">Lean axis (1=R, 2=P, 0=Y)</td>
<td><input type="text" id="leanAxis" name="leanAxis" value=""></td>
</tr>
<tr><td colspan="2" align="center"><input type="submit" style="width:150px" class="btn btn--m btn--blue" value="Save"></td></tr>
</table>
</form>
<script>



window.onload = function ()
{
	load("style.css","css", function()
	{
		load("microajax.js","js", function()
		{
				setValues("/appCFGvalues");
		});
	});
}
function load(e,t,n){if("js"==t){var a=document.createElement("script");a.src=e,a.type="text/javascript",a.async=!1,a.onload=function(){n()},document.getElementsByTagName("head")[0].appendChild(a)}else if("css"==t){var a=document.createElement("link");a.href=e,a.rel="stylesheet",a.type="text/css",a.async=!1,a.onload=function(){n()},document.getElementsByTagName("head")[0].appendChild(a)}}



</script>
)=====";


// Functions for this Page
void report_app_values()
{
	/*
	DEBUG_MSG("[Report]\n");
	DEBUG_MSG("appWifiPower:%f\n",appConfig.appWifiPower);
	DEBUG_MSG("appMinClientRSSI:%d\n", appConfig.appMinClientRSSI);
	DEBUG_MSG("appOpenRetryTime:%d\n", appConfig.appOpenRetryTime);
	DEBUG_MSG("gate._SettleTime:%d\n", appConfig.appSettleTime);
	DEBUG_MSG("gate._CloseTimeout:%d\n", appConfig.appCloseTimeout);
	DEBUG_MSG("gate._StopTime:%d\n", appConfig.appStopTime);
	DEBUG_MSG("appAutosave:%d\n", appConfig.appAutosave);
	DEBUG_MSG("appFTPuser:%s\n", appConfig.appFTPuser.c_str());
	DEBUG_MSG("appFTPpwd:%s\n", appConfig.appFTPpwd.c_str());
	DEBUG_MSG("appCFGpwd:%s\n", appConfig.appCFGpwd.c_str());
	*/
	String values ="";
	values += "brakeThrs|" +  (String)  appConfig.brakeThrs +  "|input\n";
	values += "accelThrs|" +  (String)  appConfig.accelThrs +  "|input\n";
    values += "brakeEmg|" +  (String)  appConfig.brakeEmg +  "|input\n";
    values += "accelEmg|" +  (String)  appConfig.accelEmg +  "|input\n";
    values += "accAxis|" +  (String)  appConfig.accAxis +  "|input\n";
    values += "leanAxis|" +  (String)  appConfig.leanAxis +  "|input\n";

	DEBUG_MSG("tmpSTR:%s\n\n", values.c_str());

}

void send_app_html()
{

	//DEBUG_MSG("[send_app_html] #args:%d\n",server.args());
	server.send_P ( 200, "text/html", PAGE_APPSettings );

	if (server.args() > 0 )  // Save Settings
	{
		String temp = "";
		for ( uint8_t i = 0; i < server.args(); i++ ) {
			DEBUG_MSG("[send_app_html] %s\t arg:%s::%s\n",server.uri().c_str(), server.argName(i).c_str(),server.arg(i).c_str());
			if (server.argName(i) == "brakeThrs") appConfig.brakeThrs = urldecode(server.arg(i)).toInt();
			if (server.argName(i) == "accelThrs") appConfig.accelThrs = urldecode(server.arg(i)).toInt();
            if (server.argName(i) == "brakeEmg") appConfig.brakeEmg = urldecode(server.arg(i)).toInt();
            if (server.argName(i) == "accelEmg") appConfig.accelEmg = urldecode(server.arg(i)).toInt();
            if (server.argName(i) == "accAxis") appConfig.accAxis = urldecode(server.arg(i)).toInt();
            if (server.argName(i) == "leanAxis") appConfig.leanAxis = urldecode(server.arg(i)).toInt();

		}

		report_app_values();
		WriteAppConfig();

		server.send_P ( 200, "text/html", PAGE_APPSettings );

	}
		

	//ECHO_MSG(__FUNCTION__);


}

void send_app_values_html()
{
	String values ="";
	values += "brakeThrs|" +  (String)  appConfig.brakeThrs +  "|input\n";
	values += "accelThrs|" +  (String)  appConfig.accelThrs +  "|input\n";
    values += "brakeEmg|" +  (String)  appConfig.brakeEmg +  "|input\n";
    values += "accelEmg|" +  (String)  appConfig.accelEmg +  "|input\n";
    values += "accAxis|" +  (String)  appConfig.accAxis +  "|input\n";
    values += "leanAxis|" +  (String)  appConfig.leanAxis +  "|input\n";

	server.send ( 200, "text/plain", values);
	//ECHO_MSG(__FUNCTION__);
    AdminTimeOutCounter=0;
}


