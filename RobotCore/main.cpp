#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <string>
#include <thread>
#include <pthread.h>
#include <mutex>
#include <map>
#include <sstream>
#include <vector>
#include <regex>
#include <math.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <ifaddrs.h>

using namespace std;

string useAs, myIP = "0.0.0.0";
int listening, clientSocket = 0, bufSize = 4096;
int posXYZ[3] = {0, 0, 0};
map<string, thread> gotoDict;
thread th_Received, th_setCommand, th_keyPress;
sockaddr_in client;
mutex m;

inline bool isBlank(const std::string &s)
{
    return std::all_of(s.cbegin(), s.cend(), [](char c) { return std::isspace(c); });
}
string trim_left(const string &str)
{
    const string pattern = " \f\n\r\t\v";
    return str.substr(str.find_first_not_of(pattern));
}

string trim_right(const string &str)
{
    const string pattern = " \f\n\r\t\v";
    return str.substr(0, str.find_last_not_of(pattern) + 1);
}

string trim(const string &str)
{
    return trim_left(trim_right(str));
}

string toLowers(string s)
{
    for (int i =0; i< s.size(); i++)
        s[i] = tolower(s[i], locale());
    return s;
}

string getMyIP(){
    struct ifaddrs * ifAddrStruct=NULL;
    struct ifaddrs * ifa=NULL;
    void * tmpAddrPtr=NULL;

    getifaddrs(&ifAddrStruct);
    for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr) {
            continue;
        }
        if (ifa->ifa_addr->sa_family == AF_INET) { // check it is IP4
            // is a valid IP4 Address
            tmpAddrPtr=&((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
            char addressBuffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
            if ((ifa->ifa_name) != "lo")
                myIP = addressBuffer;
            // printf("%s IP Address %s\n", ifa->ifa_name, addressBuffer); 
        } else if (ifa->ifa_addr->sa_family == AF_INET6) { // check it is IP6
            // is a valid IP6 Address
            tmpAddrPtr=&((struct sockaddr_in6 *)ifa->ifa_addr)->sin6_addr;
            char addressBuffer[INET6_ADDRSTRLEN];
            inet_ntop(AF_INET6, tmpAddrPtr, addressBuffer, INET6_ADDRSTRLEN);
            // printf("%s IP Address %s\n", ifa->ifa_name, addressBuffer); 
        } 
    }
    if (ifAddrStruct!=NULL) freeifaddrs(ifAddrStruct);
    return myIP;
}

void sendCallBack(string message)
{
    if ((!message.empty()) && (toLowers(message) != "quit"))
    {
        char buf[bufSize];

        if (clientSocket != 0)
        {
            //	Send to server
            int sendRes = send(clientSocket, trim(message).c_str(), message.size() + 1, 0);
            if (sendRes == -1)
            {
                cout << "Could not send to server! Whoops!\r\n";
                // continue;
                return;
            }
            else
            {
                cout << "@ " << "[BaseStation]" << " : " << trim(message) << endl;
            }

            // //		Wait for response
            // memset(buf, 0, bufSize);
            // int bytesReceived = recv(clientSocket, buf, bufSize, 0);
            // if (bytesReceived == -1) {
            //     cout << "There was an error getting response from server\r\n"; }
            // else {
            //     //		Display response
            //     cout << "SERVER> " << string(buf, bytesReceived) << "\r\n"; }
        }
        else 
            printf("!...Not Connected...!");
    }
}

void sendPosXYZ()
{
    string message = "E" + to_string(posXYZ[0]) + "," + to_string(posXYZ[1]) + "," + to_string(posXYZ[2]);
    sendCallBack(message);
}

void GotoLoc (string Robot, int endX, int endY, int endAngle, int shiftX, int shiftY, int shiftAngle) 
{
    try {
        cout << "# " << Robot << " : Goto >> " << "X:" << endX << " Y:" << endY << " ∠:" << endAngle << "°" << endl;
        bool chk[] = { true, true, true };
        while (chk[0] |= chk[1] |= chk[2]) {
            if (posXYZ[0] > 12000)
                posXYZ[0] = stoi (to_string(posXYZ[0]).substr (0, 4));
            if (posXYZ[1] > 9000)
                posXYZ[1] = stoi (to_string(posXYZ[1]).substr (0, 4));
            if (posXYZ[2] > 360)
                posXYZ[2] = stoi (to_string(posXYZ[2]).substr (0, 2));

            if ((posXYZ[0] > endX) && (shiftX > 0))
                shiftX *= -1;
            else if ((posXYZ[0] < endX) && (shiftX < 0))
                shiftX *= -1;
            if ((posXYZ[1] > endY) && (shiftY > 0))
                shiftY *= -1;
            else if ((posXYZ[1] < endY) && (shiftY < 0))
                shiftY *= -1;
            if ((posXYZ[2] > endAngle) && (shiftAngle > 0))
                shiftAngle *= -1;
            else if ((posXYZ[2] < endAngle) && (shiftAngle < 0))
                shiftAngle *= -1;

            if (posXYZ[0] != endX) {
                if (abs (endX - posXYZ[0]) < abs (shiftX)) // Shift not corresponding
                    shiftX = (endX - posXYZ[0]);
                posXYZ[0] += shiftX; // On process
            } else
                chk[0] = false; // Done
            if (posXYZ[1] != endY) {
                if (abs (endY - posXYZ[1]) < abs (shiftY)) // Shift not corresponding
                    shiftY = (endY - posXYZ[1]);
                posXYZ[1] += shiftY; // On process
            } else
                chk[1] = false; // Done
            if (posXYZ[2] != endAngle) {
                if (abs (endAngle - posXYZ[2]) < abs (shiftAngle)) // Shift not corresponding
                    shiftAngle = (endAngle - posXYZ[2]);
                posXYZ[2] += shiftAngle; // On process
            } else
                chk[2] = false; // Done

            sendPosXYZ();
            usleep (100000); // time per limit (microsecond)
        }
    } catch (exception e) 
    { 
        cout << "# Error GotoLoc \n\n" << e.what() << endl;
    }
}

void threadGoto (string keyName, string message) 
{
    string item;
    vector<int> dtXYZ;
    if (gotoDict.count(keyName)) {
        gotoDict[keyName].detach();
        gotoDict.erase (keyName);
    }
    for (stringstream ss(message); (getline(ss, item, ',')); (dtXYZ.push_back(stoi(item))));
    gotoDict[keyName] = thread( GotoLoc, useAs, dtXYZ[0], dtXYZ[1], dtXYZ[2], 20, 20, 1);
}

string ResponeSendCallback(string message)
{
    string respone = "", text = "", item;
    vector<string> _dtMessage;
    for (stringstream ss(message); (getline(ss, item, '|')); (_dtMessage.push_back(item)));
    
    if ((_dtMessage[0].find("!") == 0) && (_dtMessage[0].size() > 1)) {
        _dtMessage.clear();
        _dtMessage.push_back(_dtMessage[0].substr(1)); _dtMessage.push_back("Robot1,Robot2,Robot3"); }
    if ((_dtMessage[0].find("**") == 0) && (_dtMessage[0].size() > 2)) {
        respone = _dtMessage[0];
        goto broadcast; }
    else if ((_dtMessage[0].find("*") == 0) && (_dtMessage[0].size() > 1)) {
        goto multicast; }

    if (toLowers(_dtMessage[0]) == "myip") {
        text = "MyIP: "+ getMyIP();
        respone = getMyIP();
        if (clientSocket != 0)
            goto multicast;
        goto end; }
    else if (toLowers(_dtMessage[0]) == "as") {
        text = "UseAs: "+ useAs;
        goto end; }
    // else if ((_socketDict.ContainsKey("BaseStation")) && (socket.Client.RemoteEndPoint.ToString().Contains(_socketDict["BaseStation"].Client.RemoteEndPoint.ToString())))
    else if (clientSocket != 0)
    {
        // If to send Base Station socket
        /// LOCATION ///
        if (regex_match(_dtMessage[0].begin(), _dtMessage[0].end(), regex("^(go|Go|GO)[-]{0,1}[0-9]{1,4},[-]{0,1}[0-9]{1,4},[-]{0,1}[0-9]{1,4}$")))
        { //Goto Location           
            threadGoto(useAs, _dtMessage[0].substr(2));
            goto end;
        }

        /// INFORMATION ///
        else if (_dtMessage[0] == "B")
        { //Get the Ball
            respone = "B_";
            goto broadcast;
        }
        else if (_dtMessage[0] == "b")
        { //Lose the Ball
            respone = "b_";
            goto broadcast;
        }

        /// OTHERS ///
    }
    goto multicast;

broadcast:
    sendCallBack(respone + "|" + "Robot1,Robot2,Robot3");
    // sendByHostList("BaseStation", respone + "|" + "Robot1,Robot2,Robot3");
    goto end;

multicast:
    if (isBlank(respone))
        respone = _dtMessage[0];
    if (_dtMessage.size() > 1)
        sendCallBack(respone + "|" + _dtMessage[1]);
    else
        sendCallBack(respone);
    // sendByHostList("BaseStation", respone + "|" + chkRobotCollect);
    goto end;

end:
    if (!isBlank(text))
        cout << "# " << text << endl;
    return respone;
}

string ResponeReceivedCallback(string message)
{
    string respone = "", text = "", item;
    vector<string> _dtMessage, msgXYZs, msgXYZ;
    for (stringstream ss(message); (getline(ss, item, '|')); (_dtMessage.push_back(item)));
    if ((_dtMessage[0].find("!") == 0) && (_dtMessage[0].size() > 1)) {         // Broadcast message
        _dtMessage.clear();
        _dtMessage.push_back(_dtMessage[0].substr(1)); _dtMessage.push_back("Robot1,Robot2,Robot3"); }
    if ((_dtMessage[0].find("**") == 0) && (_dtMessage[0].size() > 2)) {        // Forward & Broadcast message
        respone = _dtMessage[0].substr(2);
        goto broadcast; }
    else if ((_dtMessage[0].find("*") == 0) && (_dtMessage[0].size() > 1)) {    // Forward & Multicast essage
        respone = _dtMessage[0].substr(1);
        goto multicast; }

    if ((toLowers(_dtMessage[0]).find("go") != 0) && (regex_match(_dtMessage[0].begin(), _dtMessage[0].end(), regex("E[-]{0,1}[0-9]{1,4},[-]{0,1}[0-9]{1,4},[-]{0,1}[0-9]{1,4}"))))
    {
        // Ifrespone message is data X & Y from encoder
        /// Scale is 1 : 20
        for (stringstream ss(_dtMessage[0]); (getline(ss, item, 'E')); (msgXYZs.push_back(item)));
        for (stringstream ss(msgXYZs.back()); (getline(ss, item, ',')); (msgXYZ.push_back(item)));

        for (int i = 0; i < msgXYZ.size(); i++)
            posXYZ[i] = stoi(msgXYZ[i]);
        sendPosXYZ();
        // text = "X:" + to_string(posXYZ[0]) + " Y:" + to_string(posXYZ[1]) + " ∠:" + to_string(posXYZ[2]) + "°";
    }
    // else if ((_socketDict.ContainsKey("BaseStation")) && (socket.Client.RemoteEndPoint.ToString().Contains(_socketDict["BaseStation"].Client.RemoteEndPoint.ToString())))
    else if (clientSocket != 0)
    // else if (true)
    {
        // If socket is Base Station socket
        ////    REFEREE BOX COMMANDSt	////
        {
        /// 1. DEFAULT COMMANDS ///
            if (_dtMessage[0] == "S") { //STOP
                text = "STOP"; }
            if (_dtMessage[0] == "s") { //START
                text = "START"; }
            if (_dtMessage[0] == "W") { //WELCOME (welcome message)
                text = "WELCOME"; }
            if (_dtMessage[0] == "Z") { //RESET (Reset Game)
                text = "RESET"; }
            if (_dtMessage[0] == "U") { //TESTMODE_ON (TestMode On)
                text = "TESTMODE_ON"; }
            if (_dtMessage[0] == "u") { //TESTMODE_OFF (TestMode Off)
                text = "TESTMODE_OFF"; }

        /// 3. GAME FLOW COMMANDS ///
            if (_dtMessage[0] == "1") { //FIRST_HALF
                text = "FIRST_HALF"; }
            if (_dtMessage[0] == "2") { //SECOND_HALF
                text = "SECOND_HALF"; }
            if (_dtMessage[0] == "3") { //FIRST_HALF_OVERTIME
                text = "FIRST_HALF_OVERTIME"; }
            if (_dtMessage[0] == "4") { //SECOND_HALF_OVERTIME
                text = "SECOND_HALF_OVERTIME"; }
            if (_dtMessage[0] == "h") { //HALF_TIME
                text = "HALF_TIME"; }
            if (_dtMessage[0] == "e") { //END_GAME (ends 2nd part, may go into overtime)
                text = "END_GAME"; }
            if (_dtMessage[0] == "z") { //GAMEOVER (Game Over)
                text = "GAMEOVER"; }
            if (_dtMessage[0] == "L") { //PARKING
                text = "PARKING"; }
        
        /// 2. PENALTY COMMANDS ///
            if (_dtMessage[0] == "Y") { //YELLOW_CARD_CYAN
                text = "YELLOW_CARD_CYAN"; }
            if (_dtMessage[0] == "R") { //RED_CARD_CYAN
                text = "RED_CARD_CYAN"; }
            if (_dtMessage[0] == "B") { //DOUBLE_YELLOW_CYAN
                text = "DOUBLE_YELLOW_CYAN"; }

        /// 4. GOAL STATUS ///
            if (_dtMessage[0] == "A") { //GOAL_CYAN
                text = "GOAL_CYAN"; }
            if (_dtMessage[0] == "D") { //SUBGOAL_CYAN
                text = "SUBGOAL_CYAN"; }

        /// 5. GAME FLOW COMMANDS ///
            if (_dtMessage[0] == "K") { //KICKOFF_CYAN
                text = "KICKOFF_CYAN"; }
            if (_dtMessage[0] == "F") { //FREEKICK_CYAN
                text = "FREEKICK_CYAN"; }
            if (_dtMessage[0] == "G") { //GOALKICK_CYAN
                text = "GOALKICK_CYAN"; }
            if (_dtMessage[0] == "T") { //THROWN_CYAN
                text = "THROWN_CYAN"; }
            if (_dtMessage[0] == "C") { //CORNER_CYAN
                text = "CORNER_CYAN"; }

        /// 2. PENALTY COMMANDS ///
            if (_dtMessage[0] == "y") { //YELLOW_CARD_MAGENTA	
                text = "YELLOW_CARD_MAGENTA"; }
            if (_dtMessage[0] == "r") { //RED_CARD_MAGENTA
                text = "RED_CARD_MAGENTA"; }
            if (_dtMessage[0] == "b") { //DOUBLE_YELLOW_MAGENTA
                text = "DOUBLE_YELLOW_MAGENTA"; }

        /// 4. GOAL STATUS ///
            if (_dtMessage[0] == "a") { //GOAL_MAGENTA
                text = "GOAL_MAGENTA"; }
            if (_dtMessage[0] == "d") { //SUBGOAL_MAGENTA
                text = "SUBGOAL_MAGENTA"; }

        /// 5. GAME FLOW COMMANDS ///
            if (_dtMessage[0] == "k") { //KICKOFF_MAGENTA
                text = "KICKOFF_MAGENTA"; }
            if (_dtMessage[0] == "f") { //FREEKICK_MAGENTA
                text = "FREEKICK_MAGENTA"; }
            if (_dtMessage[0] == "g") { //GOALKICK_MAGENTA
                text = "GOALKICK_MAGENTA"; }
            if (_dtMessage[0] == "t") { //THROWN_MAGENTA
                text = "THROWN_MAGENTA"; }
            if (_dtMessage[0] == "c") { //CORNER_MAGENTA
                text = "CORNER_MAGENTA"; }
        }

        /// INFORMATION ///
        if (_dtMessage[0].find("B_Robot") == 0)
        { //Get the ball
            text = "Get the ball";
        }
        else if (_dtMessage[0] == "b_")
        { //Lose the ball
            text = "Lose the ball  ";
        }

        /// OTHERS ///
        else if (toLowers(_dtMessage[0]) == "ping")
        { //PING-REPLY
            respone = "Reply " + useAs;
            goto multicast;
        }
        else if (toLowers(_dtMessage[0]) == "ip")
        { //IP Address Info
            respone = getMyIP();
            goto multicast;
        }
        else if (toLowers(_dtMessage[0]) == "get_time")
        { //TIME NOW
            time_t ct = time(0);
            respone = ctime(&ct);
            goto multicast;
        }
        // else
        //     respone = text = "# Invalid Command :<";
    }
    if ((isBlank(respone)) && (_dtMessage.size() > 1))
        sendCallBack(_dtMessage[0] + "|" + _dtMessage[1]);
    goto end;

broadcast:
    sendCallBack(respone + "|" + "Robot1,Robot2,Robot3");
    // sendByHostList("BaseStation", respone + "|" + "Robot1,Robot2,Robot3");
    goto end;

multicast:
    if (_dtMessage.size() > 1)
        sendCallBack(respone + "|" + _dtMessage[1]);
    else
        sendCallBack(respone);
    // sendByHostList("BaseStation", respone + "|" + chkRobotCollect);
    goto end;

end:
    if (!isBlank(text))
        cout << "# " << text << endl;
   return respone;
}

void receivedCallBack()
{
    string message = "";
    // for (m.lock(); (true) && (toLowers(message) != "quit"); message.clear(), m.unlock())
    while ((true) && (toLowers(message) != "quit"))
    {
        // While loop: accept and echo message back to client user Input
        char buf[bufSize];
        memset(buf, 0, bufSize);

        // Wait for client to send data
        int bytesReceived = recv(clientSocket, buf, bufSize, 0);
        if (bytesReceived == -1) {
            cerr << "Error in receivedCallBack(). Quitting" << endl;
            break; }

        if (bytesReceived == 0) {
            cout << "Client disconnected " << endl;
            break; }
        message = trim(string(buf, 0, bytesReceived));
        cout << "> " << "[BaseStation]" << " : " << message << endl;
        ResponeReceivedCallback(message);
        
        // Echo message back to client
        // send(clientSocket, buf, bytesReceived + 1, 0);
    }
    // Close the socket
    close(clientSocket);
}

void listenClient(int listening)
{
    // Wait for a connection
    socklen_t clientSize = sizeof(client);
    clientSocket = accept(listening, (sockaddr*)&client, &clientSize);

    char host[NI_MAXHOST];      // Client's remote name
    char service[NI_MAXSERV];   // Service (i.e. port) the client is connect on

    memset(host, 0, NI_MAXHOST); // same as memset(host, 0, NI_MAXHOST);
    memset(service, 0, NI_MAXSERV);

    printf("IP address is: %s\n", inet_ntoa(client.sin_addr));
    // printf("port is: %d\n", (int) ntohs(client.sin_port));

    if (getnameinfo((sockaddr*)&client, sizeof(client), host, NI_MAXHOST, service, NI_MAXSERV, 0) == 0) {
        cout << host << " connected on port " << service << endl; }
    else {
        inet_ntop(AF_INET, &client.sin_addr, host, NI_MAXHOST);
        cout << host << " connected on port " << ntohs(client.sin_port) << endl; }

    // Close listening socket
    close(listening);

    // Start received message
    th_Received = thread(receivedCallBack);
    sendCallBack(useAs);
    sendPosXYZ();
}

int setupServer(int port)
{
    cout << "Server Starting..." << endl;
    // Create a socket
    listening = socket(AF_INET, SOCK_STREAM, 0);
    if (listening == -1) {
        cerr << "Can't create a socket! Quitting" << endl;
        return -1; }

    // Bind the ip address and port to a socket
    sockaddr_in hint;
    hint.sin_family = AF_INET;
    hint.sin_port = htons(port);
    inet_pton(AF_INET, "0.0.0.0", &hint.sin_addr);
    bind(listening, (sockaddr*)&hint, sizeof(hint));

    // Tell Winsock the socket is for listening
    listen(listening, SOMAXCONN);
    cout  << "Listening to TCP clients at " << getMyIP() << " : " << port << endl;
    listenClient(listening);
}




int kbhit(void)
{
    struct termios oldt, newt;
    int ch;
    int oldf;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);
    if (ch != EOF) {
        ungetc(ch, stdin);
        return 1; }
    return 0;
}

void keyEvent(string key)
{
    int _temp[3];
    copy(begin(posXYZ), (posXYZ), begin(_temp));
    if (key == "[C")
        posXYZ[0] += 1;
    else if (key == "[D")
        posXYZ[0] -= 1;
    else if (key == "[A")
        posXYZ[1] -= 1;
    else if (key == "[B")
        posXYZ[1] += 1;
    else if (key == "[5")
        posXYZ[2] += 1;
    else if (key == "[6")
        posXYZ[2] -= 1;

    for (int i = 0; i < 3; i++)
        if ((posXYZ[i] != _temp[i]) && (clientSocket != 0) /*&& (_socketDict.ContainsKey("BaseStation"))*/){
            sendPosXYZ();
            break; }
    // cout << "# X:" << posXYZ[0] << " Y:" << posXYZ[1] << " ∠:" << posXYZ[2] << "°" << endl;
}

void keyPress()
{
    string s = "";
    int i = 0;
    printf("# Set Location Mode \n");
    for (m.lock(); 1; m.unlock()){
        if (kbhit())
            s += getchar();
        if ((s.rfind("^") != (s.size() - 1)) && (s.rfind("[") != (s.size() - 1))) {
            if (s == "") {
                if (i == 1)
                    break;
                i++; }
            else {
                keyEvent(s);
                i = 0; }
            s = ""; } }
    printf("# Set Command Mode \n");
}

void setCommand()
{
    try {        
        string Command;
        for (m.lock(); (true) && (toLowers(Command) != "quit"); m.unlock()){
            getline(cin, Command);

            if (Command == "/") {
                thread th_keyPress(keyPress);
                th_keyPress.join();
            }
            else if (!isBlank(Command))
                ResponeSendCallback(Command); }
        close(listening);
        close(clientSocket);
        cout << "# Close App" << endl;
    }
    catch (exception e) {
        cout << "% setCommand error \n~\n" << e.what() << endl; }
}

int main()
{
    for (m.lock(); isBlank(useAs); m.unlock()){
        system("clear");
        printf("~ Welcome to Robot Core ~ \n");
        printf("Use As: "); cin >> useAs;
        if ((useAs == "1") ^ (toLowers(useAs) == "r1") ^ (toLowers(useAs) == "robot1")) useAs = "Robot1";
        else if ((useAs == "2") ^ (toLowers(useAs) == "r2") ^ (toLowers(useAs) == "robot2")) useAs = "Robot2";
        else if (((useAs == "3") ^ toLowers(useAs) == "r3") ^ (toLowers(useAs) == "robot3")) useAs = "Robot3";
        else useAs.clear(); }
    getMyIP();
    setupServer(8686);
    thread th_setCommand(setCommand);
    th_setCommand.join();
    return 0;
}
