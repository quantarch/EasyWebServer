/*

EasyWebServer - A simple lightweight web server for arduino.

k@llelundberg.se - http://techblog.se/

This web server works on top of the EthernetClient class, defined in Ethernet.h.
Since there are Other network libraries (e.g. UIPEthernet.h) the headers can not
be included from this file.

Thats why this file contains both declaration and implementarion. You need include
EasyWebServer.h AFTER the Ethernet library in your source code.

*/
#ifndef EasyWebServer_h
#define EasyWebServer_h

// fix for ESP builds
#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

#define EWS_REQUEST_BUF_LEN 128 // Adjust this to support longer URLs.

class EasyWebServer;    // Forward declaration, needed for typedef below.

typedef void (*EwsRequestHandler)(EasyWebServer &w);
typedef enum {EWS_TYPE_HTML, EWS_TYPE_TEXT, EWS_TYPE_JSON} EwsContentType;

class EasyWebServer {
  public:
    const char *verb = NULL;		 // A string that will contain the VERB (GET, POST)
    const char *url = NULL;          // The URL of the request, without any querystring.
	const char *querystring = NULL;  // The querystring, everything behind the questionmark.
   char **queryVarName = NULL; // list of Strings for individual varaible names. index matches value in queryVarValue
   char **queryVarValue = NULL; // list of Strings for individual values. index matches name in queryVarName
   char *tokenizedQueryString = NULL;
   int countQueryVars = 0;
	Client& client;           // A reference to the Client object to work with.

	EasyWebServer(Client &client);
    ~EasyWebServer();

	void serveUrl(const char* url, EwsRequestHandler func, EwsContentType contentType);
	void redirect(const char* url, const char* newurl);
   const char* getValue(const char* varName);

  private:
    char _request[EWS_REQUEST_BUF_LEN]; // A buffer to store the HTTP request.
    void disconnect();                  // Close the client connection.
	void throwError(const __FlashStringHelper *error); // HTTP error response.
   void clearQueryVars();
   void parseQueryVars();
};


EasyWebServer::EasyWebServer(Client &ec):client(ec){

  /* READ THE REQUEST */
  bool firstChar = true;         // Keep track of line beginnings to see the empty line marking end of header.
  int i = 0;                     // Number of characters read.
  char c = '\0';
  while (client.connected()) {   // While the TCP session is connected.
    if (client.available()) {    // Client data available to read?

      char c = client.read();    // Read 1 byte (character) from client
      if (i < EWS_REQUEST_BUF_LEN)
        _request[i] = c;         // Add the char to the global request buffer

      i++;                       // Increase the byte counter.

      if (c == '\n') {           // If this char is a end-of-line...
        if (firstChar) break;    // If it was the first char in a line, the line is empty. Break out.
        firstChar = true;        // Note that the next char will be the first in a line.
      }

      if (c != '\n' && c != '\r') // If the char is not a line ending character...
        firstChar = false;        // ...the next char is not going to be the first char of a line.

    }
  }

  /* RESET QUERY REQUEST VARS */
  clearQueryVars();

  /* PARSE THE REQUEST */
  int rlen = min(i, EWS_REQUEST_BUF_LEN - 1); // Calculate the request length
  _request[rlen]='\0';			              // At least one string terminator in the buffer.
  verb = url = querystring = &_request[rlen]; // Reset all the strings to empty string.

  char *spc1=NULL, *spc2=NULL, *q=NULL;       // Initiate some marker variables.
  spc1 = strchr(_request,' ');                // Search for the space that separates the verb and the URL.
  if(spc1==NULL){
	  throwError(F("400 Bad Request"));
	  return;
  }
  verb = _request;
  spc1[0]='\0';								  // Terminate the verb string

  spc2 = strchr(spc1+1,' ');                  // Search for the space that after the URL
  if(spc2==NULL){
    throwError(F("414 Request-URI Too Long"));
	return;
  }
  url = spc1 + 1;
  spc2[0] = '\0';                             // Terminate the URL string.

  q = strchr(spc1+1,'?');                     // Search for a question mark inside the URL
  if(q!=NULL){
	q[0]='\0';
	querystring = q + 1;
  }

  // PARSE querystring into individual variables
  parseQueryVars();

  // Reject all HTTP verbs except GET
  if (strcmp(verb, "GET")!=0) {
    throwError(F("405 Method Not Allowed"));
    disconnect();
    return;
  }
}

void EasyWebServer::serveUrl(const char* url, EwsRequestHandler func, const EwsContentType responseContentType=EWS_TYPE_HTML){


  if(client){
	Serial.print("serveUrl");
	Serial.print(url);

    // Compare the url parameter with the url in the http request.
    if(strcmp(this->url,url)==0){

      // Write the response header
      client.println(F("HTTP/1.1 200 OK"));                 // Normal HTTP response.
      client.println(F("Connection: close"));               // Indicates to the client that the TCP connections will be closed and not reused.
      if(responseContentType==EWS_TYPE_JSON)
        client.println(F("Content-Type: application/json; charset=utf-8"));// Content type is set to JSON
      else if(responseContentType==EWS_TYPE_TEXT)
        client.println(F("Content-Type: text/plain; charset=utf-8"));      // Content type is set to plain text
      else
        client.println(F("Content-Type: text/html; charset=utf-8"));       // Content type is set to HTML (default)

      client.println();                                     // Empty line to indicate the end of the header.

      func(*this);         // Call the specified function and pass the current object as parameter.

      disconnect();          // Disconnect the TCP connection

    }
  }
}

void EasyWebServer::redirect(const char* url, const char* newurl){
  if(client){

	// Compare the url parameter with the url in the http request.
    if(strcmp(this->url,url)==0){
	  client.println(F("HTTP/1.1 301 Moved Permanently"));
	  client.println(F("Connection: close"));
	  client.print(F("Location: "));
	  client.println(newurl);
	  client.println();
	  disconnect();
	}
  }
}

EasyWebServer::~EasyWebServer(){
  if(client){
	throwError(F("404 Not Found")); // If no URL has been served, throw a 404.
  }
}

void EasyWebServer::disconnect(){
    client.flush(); // Flush the streams, make sure that the response has been delivered to the network.
    delay(1);       // Delay a little bit, shouldnt be needed if the flush() works.
    client.stop();  // Close the TCP Connection.
}

void EasyWebServer::throwError(const __FlashStringHelper* error){
    client.print(F("HTTP/1.1 "));
	client.println(error);
    client.println(F("Connection: close"));
    client.println(F("Content-Type: text/html"));
    client.println();
    client.print(F("<html><head><title>"));
	client.print(error);
	client.print(F("</title></head><body>"));
	client.print(error);
	client.print(F("</body></html>"));
    disconnect();
}

void EasyWebServer::clearQueryVars()
{
   if (NULL != queryVarName)
   {
      free(queryVarName);
   }
   queryVarName = NULL;
   if (NULL != queryVarValue)
   {
      free(queryVarValue);
   }
   queryVarValue = NULL;
   if (NULL != tokenizedQueryString)
   {
      free(tokenizedQueryString);
   }
   tokenizedQueryString = NULL;
   countQueryVars = 0;
}

void EasyWebServer::parseQueryVars()
{
   clearQueryVars();

   if (NULL != querystring)
   {
      tokenizedQueryString = new char[strlen(querystring)+1];
      char *p = tokenizedQueryString;
      strcpy(p, querystring);
      char *str;
      while ((str = strtok_r(p, "&", &p)) != NULL)
      {
         countQueryVars++;
         if (NULL == queryVarName)
         {
            queryVarName = (char**) malloc(sizeof(char*)*countQueryVars);
         }
         else
         {
            queryVarName = (char**) realloc(queryVarName, sizeof(char*)*countQueryVars);
         }
         queryVarName[countQueryVars-1] = str;
      }
      queryVarValue = (char**) malloc(sizeof(char*)*countQueryVars);
      for (int j = 0; j < countQueryVars; j++)
      {
         p = queryVarName[j];
         str = strtok_r(p, "=", &p); // terminate variable name
         queryVarValue[j] = strtok_r(p, "=", &p); // find variable value
      }
   }
}

const char* EasyWebServer::getValue(const char* varName)
{
   char* returnValue = NULL;
   for (int j = 0; j < countQueryVars; j++)
   {
      if (strcmp(varName, queryVarName[j]) == 0)
      {
         returnValue = queryVarValue[j];
      }
   }
   return returnValue;
}


#endif
