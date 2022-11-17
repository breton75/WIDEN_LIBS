﻿#ifndef SV_TCP_CLIENT_H
#define SV_TCP_CLIENT_H

#include <QNetworkInterface>
#include <QTcpSocket>

#include "tcp_client_global.h"
#include "tcp_client_defs.h"

#include "../../../../Modus/global/global_defs.h"
#include "../../../../Modus/global/device/interface/sv_abstract_interface.h"

extern "C" {

    TCP_CLIENT_EXPORT modus::SvAbstractInterface* create();

    TCP_CLIENT_EXPORT const char* getDefaultParams();
    TCP_CLIENT_EXPORT const char* getName();
    TCP_CLIENT_EXPORT const char* getDescription();
}

class SvTcpClient: public modus::SvAbstractInterface
{
public:
  SvTcpClient();

  // Получение параметров интерфейса из конфигурационного файла устрйства:
  virtual bool configure(modus::DeviceConfig* config, modus::IOBuffer*iobuffer) override;

public slots:
  // Создание и инициализация объектов; создание подключений сигналов к слотам,
  // которые необходимы для работы tcp-клиента:
  bool start() override;

  // Получение данных из сокета:
  void read() override;

  // Запись данных в сокет:
  void write(modus::BUFF* buffer) override;

private:
  // TCP-сокет клиента
  QTcpSocket*   m_client = nullptr;

 // Структура, хранящая параметры TCP-клиента:
  tcp::Params   m_params;

  // Таймер, используемый в функции "SvTcpClient::read". Коментарии - в этой функции.
  QTimer*    m_gap_timer;

  // Таймер, пр таймауту которого, мы выполняем проверку установлено ли TCP-соединение
  // с сервером:
  QTimer*  connectionCheckTimer;

  void datalog(const QByteArray& bytes, QString& message);

  // На момент запуска TCP-клиента, TCP-сервер может быть не запущен, поэтому
  // мы по таймеру будем c интервалом, указанным в параметре интерфейса, вызывать
  // функцию "checkConnection", которая
  // будет проверять установлено ли TCP-соединение и, если оно не установлено, пытаться
  // его установить:
   void checkConnection(void);


private slots:
  // Отображение в утилите "logview" ошибки сокета:
  void socketError(QAbstractSocket::SocketError err);

  // Отображение в утилите "logview" изменившегося состояния подключения:
  void stateChanged(QAbstractSocket::SocketState state);

  // После выполнения чтения из сокета испускаем сигнал "dataReaded" с некоторой задержкой.
  // Более подробное описание см. в функции "SvTcpClient::read":
  void newData(void);

  // По подключению к серверу -> отображаем информацию об этом
  // в утилите "logview:
  void connected(void);

  // По отключению от сервера -> отображаем информацию об этом
  // в утилите "logview":
  void disconnected(void);

  // Выяснение, какую команду требуется выполнить, и её выполнение.
  // Аргумент "command" - требуемая команда.
  // Возможные команды: "breakConnection" - разорвать соединение с сервером.
  void say_WorkingOut(QByteArray command);


  void emit_message(const QByteArray& bytes, sv::log::Level level, sv::log::MessageTypes type);
};

#endif // SV_TCP_CLIENT_H
