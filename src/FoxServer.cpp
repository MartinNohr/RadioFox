#include "RadioFox.h"
#include "FoxServer.h"
// #include "FoxServer.h"

void InitServer()
{
    WiFi.softAP(ssid, password);
    IPAddress myIP = WiFi.softAPIP();
    // save for the menu system
    strncpy(localIpAddress, myIP.toString().c_str(), sizeof(localIpAddress));
    Serial.print("AP IP address: ");
    Serial.println(myIP);
    server.begin();
    Serial.println("Server started");
    for (OnServerItem item : OnServerList)
    {
        server.on(item.path, item.function);
    }
    // server.on("/fupload", HTTP_POST, []() { server.send(200); }, handleFileUpload);
    // server.on("/settings/increpeat", HTTP_GET, []() { server.send(200); }, IncRepeat);
    // server.on("/settings/increpeat", HTTP_GET, IncRepeat);
    /////////////////////////// End of Request commands
    server.begin();
}

// display the homepage on the web browser
void HomePage()
{
    bWebRunning = false;
    SendHTML_Header();
    webpage += "<a href='/download'><button style=\"width:auto\">Download</button></a>";
    webpage += "<a href='/upload'><button style=\"width:auto\">Upload</button></a>";
    webpage += "<a href='/settings'><button style='width:auto'>Settings</button></a>";
    webpage += "<a href='/utilities'><button style='width:auto'>Utilities</button></a>";
    webpage += "<br><h2>" + String("Folder: ") + "/" + "</h2>";
    webpage += "<a href='/runimage'><button style='width:90%;font-size:200%;color:#00ff00'>";
    webpage += String("Run File:<br>") + "/" + "</button></a>";
    webpage += "<br><br>";
    webpage += "<br>";
    webpage += "<br><br>";
    append_page_footer();
    SendHTML_Content();
    SendHTML_Stop();
}

void load_page_header(bool bRefresh)
{
    webpage = "<!DOCTYPE html><html>";
    webpage += "<head>";
    webpage += "<title>RadioFox</title>";
    webpage += "<META name='viewport' content='width=device-width, initial-scale=1.0'>";
    if (bRefresh)
        webpage += "<META http-equiv='refresh' content='2'>";
    webpage += "<style>";
    webpage += "body{max-width:98%;margin:0 auto;font-family:arial;font-size:100%;text-align:center;color:black;background-color:#888888;}";
    webpage += "ul{list-style-type:none;margin:0.1em;padding:0;border-radius:0.17em;overflow:hidden;background-color:#EEEEEE;font-size:1em;}";
    webpage += "li{float:left;border-radius:0.17em;border-right:0.06em solid #bbb;}last-child {border-right:none;font-size:85%}";
    // fontsize was 65%, changed to 100 to make tabs easier to hit on a phone
    webpage += "li a{display: block;border-radius:0.17em;padding:0.44em 0.44em;text-decoration:none;font-size:100%}";
    webpage += "li a:hover{background-color:#DDDDDD;border-radius:0.17em;font-size:85%}";
    webpage += "section {font-size:0.88em;}";
    webpage += "h1{color:white;border-radius:0.5em;font-size:1em;padding:0.2em 0.2em;background:#444444;}";
    webpage += "h2{color:orange;font-size:1.0em;}";
    webpage += "h3{font-size:0.8em;}";
    webpage += "table{font-family:arial,sans-serif;font-size:0.9em;border-collapse:collapse;width:100%;}";
    webpage += "th,td {border:0.06em solid #dddddd;text-align:left;padding:0.3em;border-bottom:0.06em solid #dddddd;}";
    webpage += "tr:nth-child(odd) {background-color:#eeeeee;}";
    webpage += ".rcorners_n {border-radius:0.2em;background:#CCCCCC;padding:0.3em 0.3em;width:100%;color:white;font-size:75%;}";
    webpage += ".rcorners_m {border-radius:0.2em;background:#CCCCCC;padding:0.3em 0.3em;width:100%;color:white;font-size:75%;}";
    webpage += ".rcorners_w {border-radius:0.2em;background:#CCCCCC;padding:0.3em 0.3em;width:100%;color:white;font-size:75%;}";
    webpage += ".column{float:left;width:100%;height:100%;}";
    webpage += ".row:after{content:'';display:table;clear:both;}";
    webpage += "*{box-sizing:border-box;}";
    webpage += "footer{background-color:#AAAAAA; text-align:center;padding:0.3em 0.3em;border-radius:0.375em;font-size:60%;}";
    webpage += "button{border-radius:0.5em;background:#666666;padding:0.3em 0.3em;width:45%;color:white;font-size:100%;}";
    webpage += ".buttons {border-radius:0.5em;background:#666666;padding:0.3em 0.3em;width:45%;color:white;font-size:80%;}";
    webpage += ".buttonsm{border-radius:0.5em;background:#666666;padding:0.3em 0.3em;width:45%; color:white;font-size:70%;}";
    webpage += ".buttonm {border-radius:0.5em;background:#666666;padding:0.3em 0.3em;width:45%;color:white;font-size:70%;}";
    webpage += ".buttonw {border-radius:0.5em;background:#666666;padding:0.3em 0.3em;width:45%;color:white;font-size:70%;}";
    webpage += "a{font-size:75%;}";
    webpage += "p{font-size:75%;}";
    webpage += "</style></head><body>";
    webpage += "<body><h1>Radio Fox Server<br>";
    webpage + "</h1>";
}

void append_page_footer()
{
    webpage += "<ul>";
    webpage += "<li><a href='/'>Home</a></li>";
    webpage += "<li><a href='/download'>Download</a></li>";
    webpage += "<li><a href='/upload'>Upload</a></li>";
    webpage += "<li><a href='/settings'>Settings</a></li>";
    webpage += "<li><a href='/utilities'>Utilities</a></li>";
    webpage += "</ul>";
    webpage += "<footer>Radio Fox ";
    webpage += FOX_Version;
    webpage += "</footer>";
    webpage += "</body></html>";
}

void SendHTML_Header()
{
    server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    server.sendHeader("Pragma", "no-cache");
    server.sendHeader("Expires", "-1");
    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server.send(200, "text/html", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.
    load_page_header(false);
    server.sendContent(webpage);
    webpage = "";
}

void SendHTML_Content()
{
    server.sendContent(webpage);
    webpage = "";
}

void SendHTML_Stop()
{
    server.sendContent("");
    server.client().stop(); // Stop is needed because no content length was sent
}

// these are used for the list of things that can be set on the settings web page
enum WEB_SETTINGS_TYPE
{
    WST_NUMBER,    // a number value, decimals 0 for integer
    WST_BOOL,      // boolean values
    WST_STRING,    // a string of characters
    WST_TEXT_ONLY, // text that will display as H2, use to separate sections
    WST_SLIDER,    // a slider control
};
typedef WEB_SETTINGS_TYPE WEB_SETTINGS_TYPE;
struct WEB_SETTINGS
{
    WEB_SETTINGS_TYPE type; // what type of data
    bool *display;          // if not NULL, compare with displayTest to display this line
    bool displayTest;       // compare with display to see if this line should display or not
    const char *text;       // show on page
    const char *name;       // the data name
    void *data;             // a pointer to the data
    int width;              // how wide to make the field
    int decimals;           // decimals for floats, although stored as ints, also used for max string length
    int min, max;           // not used yet, TODO, but will limit range of numbers
};
typedef WEB_SETTINGS WebSettings;
WebSettings WebSettingsPage[] = {
    {(WEB_SETTINGS_TYPE)WST_TEXT_ONLY, NULL, true, "Radio Settings", NULL},
    {WST_BOOL, NULL, true, "Enable Transmit", "enable_transmit", &SystemInfo.bXmitEnable},
    {WST_BOOL, NULL, true, "Transmit Low Power", "transmit_low_power", &SystemInfo.bTxPowerLow},
    {WST_NUMBER, NULL, true, "Transmit Frequency", "transmit_frequency", &SystemInfo.nFrequency, 7, 3, 0, 0},
    //{WST_NUMBER,&ImgInfo.bFixedTime,true,"Fixed Time Value (S)","fixed_time",&ImgInfo.nFixedImageTime,4,0},
    //{WST_NUMBER,&ImgInfo.bFixedTime,false,"Column Time(mS)","column_time",&ImgInfo.nFrameHold,4,0},
    //{WST_SLIDER,&ImgInfo.bFixedTime,false,"Column Time(mS)","column_time_slider",&ImgInfo.nFrameHold,4,0,0,500},
    //{WST_NUMBER,NULL,true,"Start Delay (S)","start_delay",&ImgInfo.startDelay,4,1},
    //{WST_BOOL,NULL,true,"Upside Down","upside_down",&ImgInfo.bUpsideDown},
    //{WST_BOOL,NULL,true,"Reverse Walk (left-right)","reverse_walk",&ImgInfo.bReverseImage},
    //{WST_BOOL,NULL,true,"Play Mirror Image","mirror_image",&ImgInfo.bMirrorPlayImage},
    //{WST_NUMBER,&ImgInfo.bMirrorPlayImage,true,"Middle Mirror Delay (S)","mirror_delay",&ImgInfo.nMirrorDelay,4,1},
    //{WST_BOOL,NULL,true,"Scale Height to Fit Pixels","scale_height",&ImgInfo.bScaleHeight},
    //{WST_BOOL,NULL,true,"Double Pixels (144 to 288)","double_pixels",&ImgInfo.bDoublePixels},
    //{WST_BOOL,NULL,true,"Chain Images","chain_images",&ImgInfo.bChainFiles},
    //{WST_NUMBER,&ImgInfo.bChainFiles,true,"Chain Delay (S)","chain_delay",&ImgInfo.nChainDelay,4,1},
    //{WST_NUMBER,&ImgInfo.bChainFiles,true,"Chain Repeats","chain_repeats",&ImgInfo.nChainRepeats,4,0},
    //{WST_TEXT_ONLY,NULL,true,"File Repeat Settings",NULL},
    //{WST_NUMBER,NULL,true,"Repeat Count","repeat_count",&ImgInfo.repeatCount,4,0},
    //{WST_NUMBER,NULL,true,"Repeat Delay (S)","repeat_delay",&ImgInfo.repeatDelay,4,1},
    //{WST_TEXT_ONLY,NULL,true,"Macro Repeat Settings",NULL},
    //{WST_NUMBER,NULL,true,"Repeat Count","macro_repeat_count",&ImgInfo.nRepeatCountMacro,4,0},
    //{WST_NUMBER,NULL,true,"Repeat Delay (S)","macro_repeat_delay",&ImgInfo.nRepeatWaitMacro,4,1},
    //{WST_TEXT_ONLY,NULL,true,"LED Settings",NULL},
    //{WST_NUMBER,NULL,true,"LED Brightness (1-255)","LED_brightness",&LedInfo.nLEDBrightness,4,0},
    //{WST_BOOL,NULL,true,"Gamma Correction","gamma_correction",&LedInfo.bGammaCorrection},
    //{WST_TEXT_ONLY,NULL,true,"DMX512 Settings",NULL},
    //{WST_BOOL,NULL,true,"DMX Enabled","dmx_enabled",&SystemInfo.bRunArtNetDMX},
    //{WST_STRING,&SystemInfo.bRunArtNetDMX,true,"Art-Net Name","artnet_name",&SystemInfo.cArtNetName,14,sizeof(SystemInfo.cArtNetName)},
    //{WST_STRING,&SystemInfo.bRunArtNetDMX,true,"Network to Connect To","network_name",&SystemInfo.cNetworkName,20,sizeof(SystemInfo.cNetworkName)},
    //{WST_STRING,&SystemInfo.bRunArtNetDMX,true,"Password","password",&SystemInfo.cNetworkPassword,30,sizeof(SystemInfo.cNetworkPassword)},
    //{WST_BOOL,&SystemInfo.bRunArtNetDMX,true,"Universe Start 1 (off for 0)","universe_start",&SystemInfo.bStartUniverseOne},
};

// change the settings from the web page
void WebChangeSettings()
{
    if (server.args())
    {
        void *lastData = NULL;
        void *thisData = NULL;
        // Serial.println("argcnt: " + String(server.args()));
        for (WebSettings val : WebSettingsPage)
        {
            thisData = val.data;
            // if (thisData == lastData)
            //	continue;
            lastData = thisData;
            if (val.type != WST_BOOL && !server.hasArg(val.name))
                continue;
            // Serial.println(String(val.name) + ": ~" + server.arg(val.name) + "~");
            switch (val.type)
            {
            case WST_NUMBER:
                *(int *)(val.data) = (int)(server.arg(val.name).toDouble() * pow10(val.decimals));
                break;
            case WST_SLIDER:
                // Serial.println("slider value:" + server.arg(val.name));
                *(int *)(val.data) = (int)(server.arg(val.name).toDouble() * pow10(val.decimals));
                break;
            case WST_BOOL:
                *(bool *)(val.data) = server.arg(val.name).length() ? true : false;
                break;
            case WST_STRING:
                memset(val.data, 0, val.decimals);
                strncpy((char *)(val.data), server.arg(val.name).c_str(), val.decimals - 1);
                break;
            case WST_TEXT_ONLY:
                break;
            }
        }
    }
    // Serial.println("fixed: " + String(server.arg("fixed_time")));
    WebShowSettings();
}

void WebShowSettings()
{
    String stmp;
    double sfloat;
    bool bDoneFirst = false;
    load_page_header(false);
    // webpage += ".slidecontainer{  width: 100 %;	}";
    webpage += "<form id='allsettings' onchange='document.forms[\"allsettings\"].submit()' action='/changesettings' method='post'>";
    // webpage += "<form onchange='document.getElementById(\"settingssubmitbutton\").disabled=false' action='/changesettings' method='post'>";
    for (WebSettings val : WebSettingsPage)
    {
        if (val.display && (*(val.display) != val.displayTest))
            continue;
        if (bDoneFirst)
            webpage += "<br>";
        else
            bDoneFirst = true;
        switch (val.type)
        {
        case WST_NUMBER:
            webpage += "<label>" + String(val.text) + ": ";
            stmp = String(*(int *)(val.data));
            sfloat = stmp.toDouble() / pow10(val.decimals);
            stmp = String(sfloat, val.decimals);
            webpage += "<input type='text' name='" + String(val.name) + "' size='" + String(val.width) + "' value='" + stmp + "'>";
            webpage += "</label>";
            break;
        case WST_SLIDER:
            // webpage += "<label>" + String(val.text) + ": ";
            stmp = String(*(int *)(val.data));
            sfloat = stmp.toDouble() / pow10(val.decimals);
            stmp = String(sfloat, val.decimals);
            webpage += "<input type='range' name='" + String(val.name) + "' min='" + String(val.min) + "' max='" + String(val.max) + "' value='" + stmp + "' class='slider' id='" + val.name + "'>";
            // webpage += "</label>";
            break;
        case WST_BOOL:
            webpage += "<label>" + String(val.text) + ": ";
            webpage += "<input type='checkbox' name='" + String(val.name) + "' value='" + val.name + "'";
            if (*(bool *)(val.data))
                webpage += " checked='checked'";
            webpage += ">";
            webpage += "</label>";
            break;
        case WST_STRING:
            webpage += "<label>" + String(val.text) + ": ";
            webpage += "<input type='text' name='" + String(val.name) + "' size='" + String(val.width) + "' value='" + (char *)(val.data) + "'>";
            webpage += "</label>";
            break;
        case WST_TEXT_ONLY:
            webpage += "<h2>" + String(val.text) + "</h2>";
            bDoneFirst = false;
            break;
        }
    }
    // webpage += "<br><input type='range' name='test' min='1' max='255' value='50' class='slider' id='myRange'>";
    // webpage += "<br><br><input id='settingssubmitbutton' disabled type='submit' value='Update MIW'>";
    webpage += "</form><br>";
    // if (ImgInfo.bFixedTime) {
    //	webpage += String("<p>Fixed Image Time: ") + String(ImgInfo.nFixedImageTime) + " S";
    // }
    // else {
    //	webpage += String("<p>Column Time: ") + String(ImgInfo.nFrameHold) + " mS";
    // }
    ////IncreaseRepeatButton();
    ////DecreaseRepeatButton();
    append_page_footer();
    server.send(200, "text/html", webpage);
}

// interlock so repeat of webpage won't reboot, verifyreboot has to be called first
bool b_RebootArmed = false;
void UtilitiesPage()
{
    load_page_header(false);
    webpage += "<h2>Utilities</h2>";
    webpage += "<br><br><a href='/verifyrebootsystem'><button style='width:50%;font-size:150%;color:#ffffff'>Reboot System</button></a>";
    webpage += "<br><br>";
    append_page_footer();
    server.send(200, "text/html", webpage);
    b_RebootArmed = false;
}

// verify reboot
void VerifyRebootSystem()
{
    load_page_header(false);
    webpage += "<h2>Confirm System Reboot</h2>";
    webpage += "<a href='/utilities'><button style='width:30%;font-size:150%;color:#ffffff'>Cancel</button></a>";
    webpage += "<a href='/rebootsystem'><button style='width:30%;font-size:150%;color:#ffffff'> Reboot</button></a>";
    webpage += "<br><br>";
    append_page_footer();
    server.send(200, "text/html", webpage);
    b_RebootArmed = true;
}

// reboot system from web page
void RebootSystem()
{
    if (b_RebootArmed)
    {
        ESP.restart();
    }
    else
    {
        UtilitiesPage();
    }
}
