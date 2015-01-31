#include "sonicpitcpserver.h"
#include "mainwindow.h"

// Qt stuff
#include <QtNetwork>
#include <QTcpSocket>

#include "sonicpiserver.h"

// OSC stuff
#include "oscpkt.hh"

SonicPiTCPServer::SonicPiTCPServer(MainWindow *sonicPiWindow, OscHandler *oscHandler) : SonicPiServer(sonicPiWindow, oscHandler)
{
    tcpServer = new QTcpServer();
    buffer.clear();

    connect(tcpServer, SIGNAL(newConnection()), this, SLOT(client()));
}

void SonicPiTCPServer::startServer(){
    int PORT_NUM = 4558;
    if(tcpServer->listen(QHostAddress::LocalHost, PORT_NUM)){
      qDebug() << "Server started: " << PORT_NUM;
      handler->server_started = true;
    }
    else{
      qDebug() << "Server: not started!";
    }

 }

void SonicPiTCPServer::stopServer(){}

void SonicPiTCPServer::logError(QAbstractSocket::SocketError e){
    qDebug() << e;
}

void SonicPiTCPServer::client(){
    socket = tcpServer->nextPendingConnection();
    connect(socket, SIGNAL(readyRead()), this, SLOT(readMessage()));
    connect(socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(logError(QAbstractSocket::SocketError)));
    connect(socket, SIGNAL(disconnected()), socket, SLOT(deleteLater()));
    std::vector<char>().swap(buffer);
}

void SonicPiTCPServer::readMessage()
{
    QDataStream in(socket);
    if (blockSize == 0) {
        if (socket->bytesAvailable() < (int)sizeof(quint32)){
            return;
        }
        in >> blockSize;
    }

    qDebug() << "Blocksize: " << blockSize << "";

    if (socket->bytesAvailable() < blockSize){
        return;
    }

    buffer.resize(blockSize);
    int bytesRead = socket->read(&buffer[0], blockSize);
    if(bytesRead < 0) {
      blockSize = 0;
      return;
    }
    std::vector<char> tmp(buffer);
    tmp.swap(buffer);
    handler->oscMessage(buffer);
    blockSize = 0;
    std::vector<char>().swap(buffer);
}
