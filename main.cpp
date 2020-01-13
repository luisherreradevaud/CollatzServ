#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <memory>
#include <string.h>
#include <string>
#include <cstdlib>
#include <sstream>

using namespace std;

class CollatzServ {
    public:
        CollatzServ(const char *m_ipAddress, int puerto){
            port        = puerto;
            ipAddress   = m_ipAddress;
        };

        int init(void);
        int run(void);
        
    private:
        void buf2json( int bytesRecv, char *buf , int clientSocket);
        int collatzRec(int n);

        const char*     ipAddress;  
        int             port;           
        int             listening;
        fd_set          m_master;     

};

int main() {

    CollatzServ collatz("0.0.0.0",8083);

    if ( collatz.init() != 0 ) {
        return -1;
    }

    collatz.run();

    return 0;
}


int CollatzServ::init(void) {

    listening           = socket(AF_INET,SOCK_STREAM,0);

    if ( listening == -1 ) {
        cerr << "No se pudo crear el socket";
        return -1;
    }

    sockaddr_in hint;
    hint.sin_family     = AF_INET;
    hint.sin_port       = htons(port);

    inet_pton(AF_INET, "0.0.0.0", &hint.sin_addr);

    if ( (bind( listening, (sockaddr*)&hint, sizeof(hint)) ) == -1 ) {
        cerr << "No se puede bindear";
        return -2;
    }

    if ( ( listen(listening, SOMAXCONN)) == -1 ) {
        cerr << "No puede escuchar";
        return -3;
    }

    FD_ZERO(&m_master);
    FD_SET(listening, &m_master);

    return 0;
}

int CollatzServ::run(void) {

    sockaddr_in     client;
    socklen_t       clientSize;

    char            buf[4096];

    int             clientSocket,bytesRecv,m;

    while (1) {

        struct timeval timeout;      
        timeout.tv_sec      = 1;
        timeout.tv_usec     = 0;

        setsockopt (listening, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));

        fd_set copy         = m_master;

        int socketCount     = select(listening+1, &copy, nullptr, nullptr, nullptr);

        for (int i = 0; i < socketCount; i++ ) {

            

            //if ( FD_ISSET( i , &copy ) ) {

                cout << socketCount << endl;

                clientSocket        =  accept( listening, (sockaddr*)&client, &clientSize);
                cout << "acepto" << endl;

                if ( clientSocket == -1 ) {
                    cerr << "Problema con la conexion.";
                    return -4;
                }

                FD_SET(clientSocket, &m_master);

                memset(buf,0,4096);

                bytesRecv           = recv(clientSocket, buf, 4096, 0);

                if ( bytesRecv != 0 ) {
                    buf2json(bytesRecv,buf,clientSocket);
                }

                close(clientSocket);
            //}
        }

    } 
}


void CollatzServ::buf2json ( int bytesRecv, char * buf , int clientSocket ) {

    int             arg_begin,arg_end,m,n,j;
    string          content;

    n               = 0;
    m               = 0;
    arg_begin       = 0;
    j               = 1;


    for ( int i = 0; i < bytesRecv; i++ ) {

        if ( arg_begin == 0 ) {
            if ( buf[i] == 63 ) {
                arg_begin   = i;
            } 
        } else if ( arg_begin > 0 ) {
            if ( buf[i] == 32 ) {
                arg_end =    i-1;
                break;
            }
        }
    }

    for ( int i = arg_end; i > arg_begin; i-- ) {
        n   = n + ( buf[i] - 48 ) * j;
        j   = j * 10;
    }


    content = "{{\"value \" : \"" + to_string(n) + "\"}";

    while ( n != 1 ) {
        n   = collatzRec(n);
        content += ", {\"value\": \"" + to_string(n) + "\"}"; 
    }

    content += "}";
    
    ostringstream oss;

    oss << "HTTP/1.1 200 OK\r\n";
    oss << "Cache-Control: no-cache, private\r\n";
    oss << "Content-Type: application/json\r\n";
    oss << "Content-Length: " << content.size() << "\r\n";
    oss << "\r\n";
    oss << content;

    std::string output = oss.str();
    int size = output.size() + 1;

    send(clientSocket, output.c_str(), size, 0);


}

int CollatzServ::collatzRec(int n) {
    if ( n < 0 ) {
        return 0;
    }
    if ( n % 2 == 1 ) {
        n   = n * 3 + 1;
    } else {
        n   = n / 2;
    }
    return n;
}



