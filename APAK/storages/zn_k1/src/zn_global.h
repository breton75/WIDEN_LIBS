﻿#ifndef ZN_GLOBAL
#define ZN_GLOBAL

#include <QDataStream>
#include <QDateTime>
#include <QHostAddress>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>

#include "../../../../../svlib/SvCRC/1.1/sv_crc.h"
#include "../../../../../svlib/SvException/svexception.h"
#include "../../../../../Modus/global/global_defs.h"
#include "../../../../../svlib/SvAbstractLogger/svabstractlogger.h"

#define DEFAULT_DATETIME_FORMAT   "yyyy-MM-dd hh:mm:ss"

// имена параметров (отсутствующие в global_defs.h)
#define P_GLOBAL                  "global"
#define P_READER                  "reader"
#define P_PICKER                  "picker"
#define P_GRAPH                   "graph"
#define P_SYSTEMS                 "systems"
#define P_ZN_DATA_FILE            "zn_data_file"
#define P_TASKS                   "tasks"
#define P_MARKER                  "marker"
#define P_BEGIN                   "begin"
#define P_END                     "end"
#define P_FILE_NAME               "file_name"
#define P_ZN_MARKER               "zn_marker"
#define P_HOST                    "host"
#define P_ZONE                    "zone"
#define P_PASS                    "pass"
#define P_QUEUE_LEN               "queue_len"
#define P_WRITE_BUF               "write_buf"
#define P_ASK_ZONE_SIZE           "ask_zone_size"
#define P_START_BYTE              "start_byte"
#define P_ZONE_SIZE_GB            "zone_size_gb"
#define P_ZONE_SIZE               "zone_size_mb"
#define P_FULL_FILE_NAME          "full_file_name"
#define P_BUFF_SIZE               "buff_size_mb"

///дефолтные значения
#define DEFAULT_PORT              10000
#define DEFAULT_INTERVAL          1000
#define DEFAULT_QUEUE_LEN         1000
#define DEFAULT_WRITE_BUFFER_LEN  102400  // bytes. 100 kB

#define DEFAULT_READ_BUFFER_LEN   16      // megabytes. 16 MB
#define DEFAULT_READ_INTERVAL     5       // 1 сек.
#define DEFAULT_ZONE_SIZE_MB      3840    // 3.75 Gb

/// команды для работы с устройством
#define CMD_CONNECT               200
#define CMD_ANSWER                100
#define CMD_WRITE                 501
#define ACC_CODE_WRITE            2
#define CMD_READ                  502
#define ACC_CODE_READ             3
#define CMD_SIZE                  503

/// служебные определения
#define TAG_CONNECTION_STATE      "c"
#define TAG_AUTHORIZATION         "a"
#define TAG_WRITE_STATE           "w"

#define STATE_NO_CONNECTION       0
#define STATE_NO_AUTHORITY        0
#define STATE_NO_WRITING          0
#define STATE_NO_READING          0

#define STATE_CONNECTION_OK       1
#define STATE_AUTHORITY_OK        1
#define STATE_WRITING_OK          1
#define STATE_READING_OK          1

#define STATE_OK                  "OK"

#define DEFAULT_MARKER_TEMPLATE  "[MARKER]"
#define DEFAULT_BEGIN_TEMPLATE   "[BEGIN]"
#define DEFAULT_END_TEMPLATE     "[END]"

#define DEFAULT_FILE_NAME_TEMPLATE DEFAULT_MARKER_TEMPLATE "_" DEFAULT_BEGIN_TEMPLATE "_" DEFAULT_END_TEMPLATE

namespace zn1 {

  /// общие. структура записи
  const char  BunchMarker[4] = {char(0xAA), char(0xAA), char(0xAA), char(0xAA)};

#pragma pack(push,1)
  struct BunchHeader {

    char    marker[sizeof(zn1::BunchMarker)];
    qint64  coarseDateTime;
    quint32 dataLength;
    quint16 crc16;

    explicit BunchHeader():
      zn1::BunchHeader{0}
    {

    }

    explicit BunchHeader(qint64 coarseDateTime):
      coarseDateTime(coarseDateTime),
      dataLength(0),
      crc16(0)
    {
      memcpy(&marker[0], &zn1::BunchMarker[0], sizeof(marker));
    }

    bool fromRawData(const char* data)
    {
//qDebug() << 1;
      QByteArray b(data, sizeof(marker) + sizeof(coarseDateTime)  + sizeof(dataLength) + sizeof(crc16));
      QDataStream s(b);
      s.setByteOrder(QDataStream::LittleEndian); // !
//qDebug() << 2;
      s.readRawData(&marker[0], sizeof(marker));
//qDebug() << 3;
      s >> coarseDateTime >> dataLength >> crc16;
//qDebug() << 4;
      return (crc16 == crc::crc16ccitt(data, sizeof(marker) + sizeof(coarseDateTime) + sizeof(dataLength)));
    }

    QByteArray toByteArray(quint32 dataLength)
    {
      QByteArray result;

      QDataStream stream(&result, QIODevice::WriteOnly);
      stream.setByteOrder(QDataStream::LittleEndian);

      stream.writeRawData(&marker[0], sizeof(marker));

      stream << coarseDateTime
             << dataLength;
      quint16 crc16 = crc::crc16ccitt(result);
//      qDebug() << "toByteArray" << QString(result.toHex()) << QString::number(crc16, 16);
      stream << crc16;

      return result;

    }

    quint32 length()
    {
      return sizeof(marker) + sizeof(coarseDateTime) + sizeof(quint32) + sizeof(quint16);
    }

  };
#pragma pack(pop)

  class Record
  {
  public:
    Record():
      m_date_time(0),
      m_apak_marker(QString()),
      m_record_data(QByteArray())
    {

    }

    Record(qint64  dateTime, const QString& apak_marker, const QByteArray& data):
      m_date_time(dateTime),
      m_apak_marker(apak_marker)
    {
      QDataStream stream(&m_record_data, QIODevice::WriteOnly);
      stream.setByteOrder(QDataStream::LittleEndian);

      stream << dateTime;
      stream << static_cast<quint16>(data.length());
      stream << static_cast<quint16>(apak_marker.length());

      stream.writeRawData(apak_marker.toStdString().c_str(), apak_marker.length());
      stream.writeRawData(data.data(), data.length());

//      qDebug() << "Record" << QDateTime::fromMSecsSinceEpoch(dateTime)
//               << apak_marker << data.length();
    }

    static int headsz() { return sizeof(m_date_time) + sizeof(m_data_length) + sizeof(m_apak_marker_length); }

    bool fromRawData(const char* data)
    {
//qDebug() << 11;
      QByteArray r{data, headsz()};
      QDataStream stream(r);
      stream.setByteOrder(QDataStream::LittleEndian); // !
//qDebug() << 12;
      stream >> m_date_time >> m_data_length >> m_apak_marker_length;
//qDebug() << 13;
      /// вводим жесткие ограничения на значения, которые могут быть в заголовке записи
      if((m_date_time <= 0) || (m_data_length == 0) || (m_apak_marker_length > 32) || (m_apak_marker_length == 0)) {
        qDebug() << "fromRawData false";
        return false;
      }
//qDebug() << 14;
//      qDebug() << "apak_marker" << QByteArray(data + sz, m_apak_marker_length).toHex();

      m_apak_marker = QString::fromLocal8Bit(data + headsz(), m_apak_marker_length);
      m_record_data = QByteArray::fromRawData(data + headsz() + m_apak_marker_length, m_data_length);
//qDebug() << 15;
      return true;

//qDebug() << "Record fromRawData"
//         << QDateTime::fromMSecsSinceEpoch(m_date_time)
//         << m_apak_marker
//         << m_data_length
//         << sizeof(m_date_time) +
//            sizeof(m_data_length) +
//            sizeof(m_apak_marker_length) +
//            m_apak_marker.length() +
//            m_record_data.length();

    }

    const QByteArray& data()        const { return m_record_data;       }
    const QString&    marker()      const { return m_apak_marker;       }
    uint              marker_hash()       { return qHash(m_apak_marker);}
    qint64            dateTime()          { return m_date_time;         }
    quint16           dataLength()        { return m_data_length;       }
    quint16           markerLength()      { return m_apak_marker_length;}

//    int length() { return m_record_data.length(); }

    qint64 size() { return  sizeof(m_date_time) +
                            sizeof(m_data_length) +
                            sizeof(m_apak_marker_length) +
                            m_apak_marker.length() +
                            m_record_data.length(); }

  private:
    qint64      m_date_time;
    quint16     m_data_length;
    quint16     m_apak_marker_length;
    QString     m_apak_marker;
    QByteArray  m_record_data;


  };

  class Bunch
  {

  public:
    enum States {
      Undefined,
      Underway,
      Undelivered,
      Delivered,
      Ready
    };

    Bunch(qint64 coarseDateTime = 0, States state = Underway):
      m_header(BunchHeader(coarseDateTime)),
      m_data(QByteArray()),
      m_state(state),
      m_record_count(0)
    {

    }

    const BunchHeader header()  const { return m_header; }
    const QByteArray& data()    const { return m_data;   }
    States      state()               { return m_state;  }

    bool setHeader(const char* data)
    {
      return m_header.fromRawData(data);
    }

    void setState(States state)
    {
      m_state = state;
    }

    void appendRecord(Record* record)
    {
      m_data.append(record->data());
      m_record_count++;
    }

    QByteArray toByteArray()
    {
      QByteArray result;

      result.append(m_header.toByteArray(m_data.length())).append(m_data);

      return result;
    }

    void appendToStream(QDataStream* const stream)
    {
      QByteArray h = m_header.toByteArray(m_data.length());
      qDebug() << "appendToStream" << QString(h.toHex());
      stream->writeRawData(h.data(), int(m_header.length()));
      stream->writeRawData(m_data, int(m_data.length()));
    }

    void makeByteArray(QByteArray& ba)
    {
      ba.append(m_header.toByteArray(m_data.length())).append(m_data);
    }

    quint32 length()
    {
      return quint32(m_header.length() + m_data.length());
    }

    quint16 recordCount()
    {
      return m_record_count;
    }

  private:
    BunchHeader  m_header;
    QByteArray   m_data;
    States       m_state;

    quint16 m_record_count;

  };

  /// общие. работа с устройством

  enum ReplyCode {

    Success         = 0,
    Failure         = 1,
    NotSupportedCmd = 2,
    WrongZoneName   = 3,
    WrongPassword   = 4,
    AlreadyInUse    = 5,
    WrongParams     = 8
  };

  const QMap<ReplyCode, QString> ReplyCodeMap = {{Success,          "Успех"                       },
                                                 {Failure,          "Отказ ЗН-К1"                 },
                                                 {NotSupportedCmd,  "Команда не поддерживается"   },
                                                 {WrongZoneName,    "Неверное имя зоны данных"    },
                                                 {WrongPassword,    "Неправильный пароль"         },
                                                 {AlreadyInUse,     "Устройство уже используется" },
                                                 {WrongParams,      "Неверные параметры команды" }};


  /// авторизация
  struct AuthorizeRequest
  {
    AuthorizeRequest(QString zone_name, QString pass, quint16 access_code, quint16 cmd = CMD_CONNECT):
      cmd(cmd),
      zone_name(zone_name),
      pass(pass),
      access_code(access_code)
    {
      length = 4 + zone_name.length() + 4 + pass.length() + 2;
    }

    QByteArray toByteArray()
    {
      QByteArray result;

      QDataStream stream(&result, QIODevice::ReadWrite);
      stream.setByteOrder(QDataStream::LittleEndian); // !
      stream << length
             << cmd
             << static_cast<quint32>(zone_name.length());

      stream.writeRawData(zone_name.toStdString().c_str(), zone_name.length());

      stream << static_cast<quint32>(pass.length());

      stream.writeRawData(pass.toStdString().c_str(), pass.length());

      stream << access_code;

      return result;

    }

    quint32     length;
    quint16     cmd;
    QString     zone_name;
    QString     pass;
    quint16     access_code;

  };

  struct AuthorizeReply
  {
    AuthorizeReply():
      length(0),
      reply_code(0),
      request_code(0),
      result(0),
      additional(QByteArray())
    {    }

    static AuthorizeReply parse(const QByteArray& ba)
    {
      AuthorizeReply result;

      if(ba.size() < 10)
        return result;

      QDataStream stream(ba);
      stream.setByteOrder(QDataStream::LittleEndian); //!

      stream >> result.length
             >> result.reply_code
             >> result.request_code
             >> result.result;

      result.additional = ba.right(result.length - 10); // согласно протокола

      return result;

    }

    quint32     length;
    quint16     reply_code;
    quint16     request_code;
    quint16     result;
    QByteArray  additional;

  };

  /// запись данных
  struct WriteRequest
  {
    WriteRequest(const QByteArray& data):
      cmd(CMD_WRITE),
      data(data)
    {
      length = data.length();
    }

    QByteArray toByteArray()
    {
      QByteArray result;

      QDataStream stream(&result, QIODevice::ReadWrite);
      stream.setByteOrder(QDataStream::LittleEndian); // !
      stream << length << cmd;

      result.append(data);

      return result;

    }

    quint32     length;
    quint16     cmd;
    QByteArray  data;

  };

  struct WriteReply
  {
    WriteReply():
      length(0),
      reply_code(0),
      request_code(0),
      result(0),
      additional(QByteArray())
    {    }

    static WriteReply parse(const QByteArray& ba)
    {
      WriteReply result;

      if(ba.length() < 10)
        return result;

      QDataStream stream(ba);
      stream.setByteOrder(QDataStream::LittleEndian); //!

      stream >> result.length
             >> result.reply_code
             >> result.request_code
             >> result.result;

      result.additional = ba.right(result.length - 10); // согласно протокола

      return result;

    }

    quint32     length;
    quint16     reply_code;
    quint16     request_code;
    quint16     result;
    QByteArray  additional;

  };

  /// чтение данных
#pragma pack(push,1)
  struct ReadRequest
  {
    ReadRequest(quint64 start_byte, quint64 byte_count):
      length(16),
      cmd(CMD_READ),
      start_byte(start_byte),
      byte_count(byte_count)
    {

    }

    QByteArray toByteArray()
    {
      QByteArray result;

      QDataStream stream(&result, QIODevice::ReadWrite);
      stream.setByteOrder(QDataStream::LittleEndian); // !
      stream << length << cmd << start_byte << byte_count;

      return result;

    }

    quint32 length;
    quint16 cmd;
    quint64 start_byte;
    quint64 byte_count;

  };
#pragma pack(pop)

#pragma pack(push,1)
  struct ReadReply
  {
    ReadReply():
      failed(false),
      length(0),
      reply_code(0),
      request_code(0),
      result(0),
      start_byte(0),
      data(QByteArray())
    {    }

    static ReadReply parse(const QByteArray& ba)
    {
      ReadReply result;

      if(ba.length() < 10) {

        result.failed = true;
        return result;
      }

      QDataStream stream(ba);
      stream.setByteOrder(QDataStream::LittleEndian); //!

      stream >> result.length           // здесь общий размер запрошенных данных, а не одного пакета!
             >> result.reply_code
             >> result.request_code
             >> result.result;

      if(result.result == ReplyCode::Success) {

        if(ba.length() > 17) {

          stream >> result.start_byte;
          result.data = ba.right(ba.size() - 18); // согласно протокола
//          qDebug() << "result: length" << result.length << "data size" << result.data.size();

        }
        else
          result.failed = true;

      }

      return result;

    }

    bool        failed;
    quint32     length;
    quint16     reply_code;
    quint16     request_code;
    quint16     result;
    quint64     start_byte;
    QByteArray  data;

  };
#pragma pack(pop)

  /// запрос размера данных
  struct DataSizeRequest
  {
    DataSizeRequest():
      cmd(CMD_SIZE)
    {
      length = 0;
    }

    QByteArray toByteArray()
    {
      QByteArray result;

      QDataStream stream(&result, QIODevice::ReadWrite);
      stream.setByteOrder(QDataStream::LittleEndian); // !
      stream << length << cmd;

      return result;

    }

    quint32     length;
    quint16     cmd;

  };

  struct DataSizeReply
  {
    DataSizeReply():
      failed(false),
      length(0),
      reply_code(0),
      request_code(0),
      result(0),
      data_size(0),
      data_pointer(0),
      additional(QByteArray())
    {    }

    static DataSizeReply parse(const QByteArray& ba)
    {
      DataSizeReply result;

      if(ba.length() < 10) { // согласно протокола

        result.failed = true;
        return result;
      }

      QDataStream stream(ba);
      stream.setByteOrder(QDataStream::LittleEndian); //!

      stream >> result.length
             >> result.reply_code
             >> result.request_code
             >> result.result;

      if(result.result == ReplyCode::Success) {

         if(ba.length() > 25) {

           stream >> result.data_size
                  >> result.data_pointer;

           result.additional = ba.right(result.length - 26); // согласно протокола

         }
         else
           result.failed = true;

      }

      return result;

    }

    bool        failed;
    quint32     length;
    quint16     reply_code;
    quint16     request_code;
    quint16     result;
    quint64     data_size;
    quint64     data_pointer;
    QByteArray  additional;

  };


  /*! структуры для описания и хранения задач для извлечения
   * необходимых данных из общего массива данных */
  class TaskPeriod
  {
  public:
    explicit TaskPeriod():
      m_begin(QDateTime::currentDateTime()),
      m_end(QDateTime::currentDateTime())
    {  }

    explicit TaskPeriod(QDateTime begin, QDateTime end):
      m_begin(begin),
      m_end(end)
    {

    }

    void setBegin(QDateTime begin)
    {
      m_begin = begin;
    }

    void setEnd(QDateTime end)
    {
      m_end = end;
    }

    void setData(QDateTime begin, QDateTime end)
    {
      m_begin = begin;
      m_end   = end;

    }

    QDateTime begin() { return m_begin; }
    QDateTime end()   { return m_end; }

    bool isValid() { return !m_begin.isNull() && m_begin.isValid() && !m_end.isNull() && m_end.isValid(); }
    bool contains(QDateTime point) { return ((point >= m_begin) && (point <= m_end)); }
    bool contains(qint64 point) const { return ((point >= m_begin.toMSecsSinceEpoch()) && (point <= m_end.toMSecsSinceEpoch())); }

    bool operator==(const TaskPeriod& other) { return (other.m_begin == m_begin) && (other.m_end == m_end); }

  private:
    QDateTime m_begin;
    QDateTime m_end;

  };

  class Task {

  public:
    Task():
      m_id(0),
      m_marker(QString()),
      m_begin(QDateTime::currentDateTime()),
      m_end(QDateTime::currentDateTime()),
      m_path(QString()),
      m_file_name(QString())
    {
      m_period.setData(m_begin, m_end);
    }

    Task(Task* task):
      m_id(task->id()),
      m_marker(task->marker()),
      m_begin(task->begin()),
      m_end(task->end()),
      m_path(task->path()),
      m_file_name(task->file_name())
    {
      m_period.setData(m_begin, m_end);
    }

    Task& operator=(const Task& other)
    {
      if(this == &other)
        return *this;

      m_id = other.m_id;
      m_marker = other.m_marker;
      m_begin = other.m_begin;
      m_end = other.m_end;
      m_path = other.m_path;
      m_file_name = other.m_file_name;
      m_period.setData(m_begin, m_end);

      return *this;
    }

    qint64      id()                const { return m_id;            }
    uint        marker_hash()             { return qHash(m_marker); }

    const QString     marker()      const { return m_marker;        }
    const QString     path()        const { return m_path;          }
    const QString     file_name()   const { return m_file_name;     }
    const QDateTime   begin()       const { return m_begin;         }
    const QDateTime   end()         const { return m_end;           }

    const TaskPeriod& period()      const { return m_period;        }

    QString   save_path()
    {
      return QDir(m_path.replace(DEFAULT_MARKER_TEMPLATE,  m_marker, Qt::CaseInsensitive)).filePath(m_file_name
                                                                  .replace(DEFAULT_MARKER_TEMPLATE, m_marker, Qt::CaseInsensitive)
                                                                  .replace(DEFAULT_BEGIN_TEMPLATE,  m_begin.toString("ddMMyyyy-hhmmss"), Qt::CaseInsensitive)
                                                                  .replace(DEFAULT_END_TEMPLATE,    m_end.toString("ddMMyyyy-hhmmss"), Qt::CaseInsensitive)
                                                                .append(m_file_name.endsWith(".znr") ? "" : ".znr"));
    }

    void setId(quint64 id)
    {
      m_id    = id;
    }

    void setBegin(QDateTime begin)
    {
      m_begin   = begin;
      m_period  = TaskPeriod(m_begin, m_end);
    }

    void setEnd(QDateTime end)
    {
      m_end     = end;
      m_period  = TaskPeriod(m_begin, m_end);
    }

    void setPeriod(QDateTime begin, QDateTime end)
    {
      m_begin   = begin;
      m_end     = end;
      m_period  = TaskPeriod(m_begin, m_end);
    }

    void setPeriod(TaskPeriod period)
    {
      setPeriod(period.begin(), period.end());
    }

    void setMarker(const QString& marker)
    {
      m_marker    = marker;
    }

    void setPath(const QString& path)
    {
      m_path    = path;
    }

    void setFileName(const QString& file_name)
    {
      m_file_name = file_name;
    }

    void setData(const QString& marker, TaskPeriod period, const QString& path, const QString& file_name)
    {
      setData(marker, period.begin(), period.end(), path, file_name);
    }

    void setData(const QString& marker, QDateTime begin, QDateTime end, const QString& path, const QString& file_name)
    {
      setPeriod(begin, end);

      m_marker    = marker;
      m_path      = path;
      m_file_name = file_name;
    }

  private:

    qint64      m_id;
    QString     m_marker;
    QDateTime   m_begin;
    QDateTime   m_end;
    QString     m_path;
    QString     m_file_name;
    TaskPeriod  m_period;

  };


  /*! структуры для хранения конфигурации:
   * ReaderParams - параметры утилиты чтения данных из черного ящика
   * PickerParams - параметры утилиты-сборщика данных
   * GraphParams - параметры программы графического тотбражения архивных данных
   * GlobalParams - общие параметры для всех утилит
   **/
  struct ReaderParams {

    QHostAddress    host            = QHostAddress();
    quint16         port            = DEFAULT_PORT;
    QString         zone            = QString();
    QString         pass            = QString();

    quint64         start_byte      = 0;
    quint32         timeout         = DEFAULT_READ_INTERVAL;

    bool            ask_zone_size   = true;
    double          zone_size_gb    = 0.0;
    quint64         zone_size_mb    = 0;
    quint64         buff_size_mb    = DEFAULT_READ_BUFFER_LEN;  // в мегабайтах // 16 mb

    static ReaderParams fromJsonString(const QString& json_string) //throw (SvException)
    {
      QJsonParseError err;
      QJsonDocument jd = QJsonDocument::fromJson(json_string.toUtf8(), &err);

      if(err.error != QJsonParseError::NoError)
        throw SvException(err.errorString());

      try {
        return fromJsonObject(jd.object());
      }
      catch(SvException& e) {
        throw e;
      }
    }

    static ReaderParams fromJsonObject(const QJsonObject &object) //throw (SvException)
    {
      ReaderParams p;
      QString P;

      if(!object.contains(P_PARAMS) || !object.value(P_PARAMS).isObject())
        throw SvException(QString("Неверная конфигурация json. Раздел \"%1\" отсутствует или не является объектом").arg(P_PARAMS));

      QJsonObject params = object.value(P_PARAMS).toObject();

      // params
      {
        // host
        P = P_HOST;
        if(params.contains(P)) {

          QString host = params.value(P).toString("").toLower();

          if(QHostAddress(host).toIPv4Address() == 0)
            throw SvException(QString(IMPERMISSIBLE_VALUE)
                               .arg(P).arg(QString(QJsonDocument(params).toJson(QJsonDocument::Compact)))
                               .arg("Допускаются только ip адреса в формате 'xxx.xxx.xxx.xxx' (192.168.1.1)"));

          p.host = QHostAddress(host);

        }
        else
          throw SvException(QString(MISSING_PARAM).arg(P));

        // receive port
        P = P_PORT;
        if(params.contains(P))
        {
          if(params.value(P).toInt(-1) < 1)
            throw SvException(QString(IMPERMISSIBLE_VALUE)
                               .arg(P).arg(QString(QJsonDocument(params).toJson(QJsonDocument::Compact)))
                               .arg("Номер порта должен быть задан целым положительным числом в диапазоне [1..65535]"));

          p.port = params.value(P).toInt(DEFAULT_PORT);

        }
        else
          throw SvException(QString(MISSING_PARAM).arg(P));

        // zone name
        P = P_ZONE;
        if(params.contains(P)) {

          p.zone = params.value(P).toString();

          if(p.zone.isEmpty())
            throw SvException(QString(IMPERMISSIBLE_VALUE)
                               .arg(P).arg(QString(QJsonDocument(params).toJson(QJsonDocument::Compact)))
                               .arg("Имя зоны для записи не может быть пустым"));
        }
        else
          throw SvException(QString(MISSING_PARAM_DESC).arg(QString(QJsonDocument(params).toJson(QJsonDocument::Compact))).arg(P));

        // read biffer length
        P = P_BUFF_SIZE;
        if(params.contains(P))
        {
          if(params.value(P).toInt(-1) < 1)
            throw SvException(QString(IMPERMISSIBLE_VALUE)
                               .arg(P).arg(QString(QJsonDocument(params).toJson(QJsonDocument::Compact)))
                               .arg("Размер буфера записи должен быть задан целым положительным числом в мегабайтах"));

          p.buff_size_mb = params.value(P).toInt(DEFAULT_READ_BUFFER_LEN);

        }
        else
          p.buff_size_mb = DEFAULT_READ_BUFFER_LEN;

        // full_file_name
        P = P_FULL_FILE_NAME;
        if(params.contains(P)) {

          p.full_file_name = params.value(P).toString();

        }
        else
          p.full_file_name = "";

        // start reading byte
        P = P_START_BYTE;
        if(params.contains(P))
        {
          if(params.value(P).toInt(-1) < 0)
            throw SvException(QString(IMPERMISSIBLE_VALUE)
                               .arg(P).arg(QString(QJsonDocument(params).toJson(QJsonDocument::Compact)))
                               .arg("Номер первого читаемого байта должен быть задан целым положительным числом"));

          p.start_byte = params.value(P).toInt(0);

        }
        else
          p.start_byte = 0;

        // timeout
        P = P_TIMEOUT;
        if(params.contains(P))
        {
          if(params.value(P).toInt(-1) < 1)
            throw SvException(QString(IMPERMISSIBLE_VALUE)
                               .arg(P).arg(QString(QJsonDocument(params).toJson(QJsonDocument::Compact)))
                               .arg("Таймаут должен быть задан целым положительным числом в секундах"));

          p.timeout = params.value(P).toInt(DEFAULT_TIMEOUT);

        }
        else
          p.timeout = DEFAULT_TIMEOUT;


        // log_level
        P = P_LOG_LEVEL;
        if(params.contains(P))
        {
          p.log_level = sv::log::stringToLevel(params.value(P).toString(""));

          if(p.log_level == sv::log::llUndefined)
            throw SvException(QString(IMPERMISSIBLE_VALUE)
                               .arg(P).arg(QString(QJsonDocument(params).toJson(QJsonDocument::Compact)))
                               .arg(QString("Неверное значение для параметра \"%1\". Допустимые значения: error, warning, info, debug, debug2")
                                                  .arg(P_LOG_LEVEL)));
        }
        else
          p.log_level = sv::log::llInfo;

      }

      // request for zone size
      P = P_ASK_ZONE_SIZE;
      if(params.contains(P)) {

        p.ask_zone_size = params.value(P).toBool(true);

      }
      else
        p.ask_zone_size = true;

      // zone_size_gb
      P = P_ZONE_SIZE_GB;
      if(params.contains(P))
      {
//        qDebug() << params.value(P).type();
        if(params.value(P).toDouble(0) <= 0)
          throw SvException(QString(IMPERMISSIBLE_VALUE)
                             .arg(P).arg(QString(QJsonDocument(params).toJson(QJsonDocument::Compact)))
                             .arg("Неверное значение размера зоны. Размер зоны должен быть задан числом в гигабайтах"));

        p.zone_size_gb  = params.value(P).toDouble();
        p.zone_size_mb     = p.zone_size_gb * 1024 * 1024 * 1024;

      }
      else
        p.zone_size_mb = 0;

      return p;

    }

    QByteArray toByteArray(QJsonDocument::JsonFormat format = QJsonDocument::Indented) const
    {
      QJsonDocument jd;
      jd.setObject(toJsonObject());

      return jd.toJson(format);
    }

    QString toJsonString(QJsonDocument::JsonFormat format = QJsonDocument::Indented) const
    {
      return QString(toByteArray(format));
    }

    QJsonObject toJsonObject() const
    {
      QJsonObject j;

      QJsonObject p;
      p.insert(P_HOST,            QJsonValue(host.toString()).toString());
      p.insert(P_PORT,            QJsonValue(static_cast<int>(port)).toInt());
      p.insert(P_ZONE,            QJsonValue(zone).toString());
      p.insert(P_PASS,            QJsonValue(pass).toString());
      p.insert(P_ZONE_SIZE_GB,    QJsonValue(static_cast<double>(zone_size_gb)).toDouble());
      p.insert(P_TIMEOUT,         QJsonValue(static_cast<int>(timeout)).toInt());
      p.insert(P_ASK_ZONE_SIZE,   QJsonValue(ask_zone_size).toBool());
      p.insert(P_FULL_FILE_NAME,  QJsonValue(full_file_name).toString());
      p.insert(P_START_BYTE,      QJsonValue(static_cast<int>(start_byte)).toInt());
      p.insert(P_LOG_LEVEL,       QJsonValue(sv::log::levelToString(log_level)).toString());
      p.insert(P_BUFF_SIZE,       QJsonValue(static_cast<int>(buff_size_mb)).toInt());

      j.insert(P_PARAMS,  QJsonValue(p));

      return j;

    }
  };

  struct PickerParams {

    qint64          start_byte      = 0;

    QList<Task>  tasks     = QList<Task>();

    static PickerParams fromJsonString(const QString& json_string) //throw (SvException)
    {
      QJsonParseError err;
      QJsonDocument jd = QJsonDocument::fromJson(json_string.toUtf8(), &err);

      if(err.error != QJsonParseError::NoError)
        throw SvException(err.errorString());

      try {
        return fromJsonObject(jd.object());
      }
      catch(SvException& e) {
        throw e;
      }
    }

    static PickerParams fromJsonObject(const QJsonObject &object) //throw (SvException)
    {
      PickerParams p;
      QString P;

      if(!object.contains(P_PARAMS) || !object.value(P_PARAMS).isObject())
        throw SvException(QString("Неверная конфигурация json. Раздел \"%1\" отсутствует или не является объектом").arg(P_PARAMS));

      QJsonObject params = object.value(P_PARAMS).toObject();

      // params
      {
//        // full_file_name
//        P = P_FULL_FILE_NAME;
//        if(params.contains(P)) {

//          p.full_file_name = params.value(P).toString();

//        }
//        else
//          p.full_file_name = "";

        // start reading byte
        P = P_START_BYTE;
        if(params.contains(P))
        {
          bool ok;
          p.start_byte = params.value(P).toVariant().toLongLong(&ok);

          if(!ok)
            throw SvException(QString(IMPERMISSIBLE_VALUE)
                               .arg(P).arg(QString(QJsonDocument(params).toJson(QJsonDocument::Compact)))
                               .arg("Номер первого читаемого байта должен быть задан целым положительным числом"));

        }
        else
          p.start_byte = 0;

//        // log_level
//        P = P_LOG_LEVEL;
//        if(params.contains(P))
//        {
//          p.log_level = sv::log::stringToLevel(params.value(P).toString(""));

//          if(p.log_level == sv::log::llUndefined)
//            throw SvException(QString(IMPERMISSIBLE_VALUE)
//                               .arg(P).arg(QString(QJsonDocument(params).toJson(QJsonDocument::Compact)))
//                               .arg(QString("Неверное значение для параметра \"%1\". Допустимые значения: error, warning, info, debug, debug2")
//                                                  .arg(P_LOG_LEVEL)));
//        }
//        else
//          p.log_level = sv::log::llInfo;

      }


      if(object.contains(P_TASKS)) {

        if(!object.value(P_TASKS).isArray())
          throw SvException(QString("Неверная конфигурация json. Раздел \"%1\" отсутствует или не является массивом").arg(P_TASKS));

        else
        {
          QJsonArray tasks = object.value(P_TASKS).toArray();

          for(QJsonValue t: tasks) {

            if(!t.isObject())
              continue;

            QJsonObject to = t.toObject();
            Task task = Task{};

            // task id
            P = P_ID;
            if(to.contains(P))
            {
              bool ok;
              qint64 id = to.value(P).toVariant().toLongLong(&ok);
              if(!ok)
                throw SvException(QString(IMPERMISSIBLE_VALUE)
                                   .arg(P).arg(QString(QJsonDocument(to).toJson(QJsonDocument::Compact)))
                                   .arg("Идентификатор задачи должен быть задан целым положительным числом"));

              for(zn1::Task task: p.tasks) {

                if(task.id() == id)
                  throw SvException(QString(IMPERMISSIBLE_VALUE)
                                     .arg(P).arg(QString(QJsonDocument(to).toJson(QJsonDocument::Compact)))
                                     .arg(QString("Не уникальный идентификатор задачи: %1").arg(id)));
              }

              task.setId(id);

            }
            else
              task.setId(QDateTime::currentMSecsSinceEpoch());

            // marker
            P = P_MARKER;
            if(to.contains(P)) {

              QString m = to.value(P).toString("");

              if(m.isEmpty())
                throw SvException(QString(IMPERMISSIBLE_VALUE)
                                   .arg(P).arg(QString(QJsonDocument(to).toJson(QJsonDocument::Compact)))
                                   .arg("Не допустим пустой маркер"));

              task.setMarker(m);

            }
            else
              throw SvException(QString(MISSING_PARAM_DESC).arg(QString(QJsonDocument(to).toJson(QJsonDocument::Compact))).arg(P));

            // begin
            P = P_BEGIN;
            if(to.contains(P)) {

              QDateTime b = QDateTime::fromString(to.value(P).toString(""), DEFAULT_DATETIME_FORMAT);

              if(b.isNull() || !b.isValid())
                throw SvException(QString(IMPERMISSIBLE_VALUE)
                                   .arg(P).arg(QString(QJsonDocument(to).toJson(QJsonDocument::Compact)))
                                   .arg("Не допустимое значение начала периода"));

              task.setBegin(b);

            }
            else
              throw SvException(QString(MISSING_PARAM_DESC).arg(QString(QJsonDocument(to).toJson(QJsonDocument::Compact))).arg(P));

            // end
            P = P_END;
            if(to.contains(P)) {

              QDateTime e = QDateTime::fromString(to.value(P).toString(""), DEFAULT_DATETIME_FORMAT);

              if(e.isNull() || !e.isValid())
                throw SvException(QString(IMPERMISSIBLE_VALUE)
                                   .arg(P).arg(QString(QJsonDocument(to).toJson(QJsonDocument::Compact)))
                                   .arg("Не допустимое значение конца периода"));

              task.setEnd(e);

            }
            else
              throw SvException(QString(MISSING_PARAM_DESC).arg(QString(QJsonDocument(to).toJson(QJsonDocument::Compact))).arg(P));

            // path
            P = P_PATH;
            if(to.contains(P)) {

              QString path = to.value(P).toString("");

              if(path.isEmpty())
                throw SvException(QString(IMPERMISSIBLE_VALUE)
                                   .arg(P).arg(QString(QJsonDocument(to).toJson(QJsonDocument::Compact)))
                                   .arg("Необходимо указать путь для сохранения данных"));

              task.setPath(path);

            }
            else
              throw SvException(QString(MISSING_PARAM_DESC).arg(QString(QJsonDocument(to).toJson(QJsonDocument::Compact))).arg(P));

            // save_file
            P = P_FILE_NAME;
            if(to.contains(P)) {

              QString file_name = to.value(P).toString("");

              if(file_name.isEmpty())
                throw SvException(QString(IMPERMISSIBLE_VALUE)
                                   .arg(P).arg(QString(QJsonDocument(to).toJson(QJsonDocument::Compact)))
                                   .arg("Необходимо указать имя файла для сохранения загруженных данных"));

              task.setFileName(file_name);

            }
            else
              throw SvException(QString(MISSING_PARAM_DESC).arg(QString(QJsonDocument(to).toJson(QJsonDocument::Compact))).arg(P));

            p.tasks.append(task);

          }
        }
      }

      return p;

    }

    QByteArray toByteArray(QJsonDocument::JsonFormat format = QJsonDocument::Indented) const
    {
      QJsonDocument jd;
      jd.setObject(toJsonObject());

      return jd.toJson(format);
    }

    QString toJsonString(QJsonDocument::JsonFormat format = QJsonDocument::Indented) const
    {
      return QString(toByteArray(format));
    }

    QJsonObject toJsonObject() const
    {
      QJsonObject j;

      QJsonObject p;
      p.insert(P_FULL_FILE_NAME, QJsonValue(full_file_name).toString());
      p.insert(P_LOG_LEVEL,      QJsonValue(sv::log::levelToString(log_level)).toString());
      p.insert(P_START_BYTE,     QJsonValue(start_byte));

      QJsonArray a;
      for(const Task& task: tasks) {

        QJsonObject t;

        t.insert(P_ID,        task.id());
        t.insert(P_MARKER,    task.marker());
        t.insert(P_BEGIN,     task.begin().toString(DEFAULT_DATETIME_FORMAT));
        t.insert(P_END,       task.end().toString(DEFAULT_DATETIME_FORMAT));
        t.insert(P_PATH,      task.path());
        t.insert(P_FILE_NAME, task.file_name());

        a.append(t);
      }

      j.insert(P_PARAMS,  QJsonValue(p));
      j.insert(P_TASKS,   QJsonValue(a));

      return j;

    }
  };

  struct GraphParams {

  };

  struct ZNSystem {
    QString marker        = "";
    QString lib           = "";
    QString signals_file  = "";
    QString description   = "";

    static ZNSystem fromJsonString(const QString& json_string) //throw (SvException)
    {
      QJsonParseError err;
      QJsonDocument jd = QJsonDocument::fromJson(json_string.toUtf8(), &err);

      if(err.error != QJsonParseError::NoError)
        throw SvException(err.errorString());

      try {
        return fromJsonObject(jd.object());
      }
      catch(SvException& e) {
        throw e;
      }
    }

    static ZNSystem fromJsonObject(const QJsonObject &object) //throw (SvException)
    {
      ZNSystem p;
      QString P;

      P = P_MARKER;
      if(global.contains(P)) {

        p.marker = global.value(P).toString();

      }

      P = P_LIB;
      if(global.contains(P)) {

        p.marker = global.value(P).toString();

      }

      P = P_SIGNALS;
      if(global.contains(P)) {

        p.signals_file = global.value(P).toString();

      }

      P = P_DESCRIPTION;
      if(global.contains(P)) {

        p.description = global.value(P).toString();

      }

      return p;

    }

    QByteArray toByteArray(QJsonDocument::JsonFormat format = QJsonDocument::Indented) const
    {
      QJsonDocument jd;
      jd.setObject(toJsonObject());

      return jd.toJson(format);
    }

    QString toJsonString(QJsonDocument::JsonFormat format = QJsonDocument::Indented) const
    {
      return QString(toByteArray(format));
    }

    QJsonObject toJsonObject() const
    {
      QJsonObject p;

      p.insert(P_MARKER,      QJsonValue(marker).toString());
      p.insert(P_LIB,         QJsonValue(lib);
      p.insert(P_SIGNALS,     QJsonValue(signals_file));
      p.insert(P_DESCRIPTION, QJsonValue(description));

      return p;

    }
  };

  struct GlobalParams {

    ReaderParams readerParams;
    PickerParams peackerParams;
    GraphParams  graphParams;

    sv::log::Level  log_level     = sv::log::llInfo;
    QString         zn_data_file  = QString();

    QList<ZNSystem> systems       = QList<ZNSystem>();


    static GlobalParams fromJsonString(const QString& json_string) //throw (SvException)
    {
      QJsonParseError err;
      QJsonDocument jd = QJsonDocument::fromJson(json_string.toUtf8(), &err);

      if(err.error != QJsonParseError::NoError)
        throw SvException(err.errorString());

      try {
        return fromJsonObject(jd.object());
      }
      catch(SvException& e) {
        throw e;
      }
    }

    static GlobalParams fromJsonObject(const QJsonObject &object) //throw (SvException)
    {
      GlobalParams p;
      QString P;

      if(!object.contains(P_GLOBAL) || !object.value(P_GLOBAL).isObject())
        throw SvException(QString("Неверная конфигурация json. Раздел \"%1\" отсутствует или не является объектом").arg(P_PARAMS));

      QJsonObject global = object.value(P_GLOBAL).toObject();

      // params
      {

        // zn_data_file
        P = P_ZN_DATA_FILE;
        if(global.contains(P)) {

          p.zn_data_file = global.value(P).toString();

        }
        else
          p.zn_data_file = "";

        // log_level
        P = P_LOG_LEVEL;
        if(global.contains(P))
        {
          p.log_level = sv::log::stringToLevel(global.value(P).toString(""));

          if(p.log_level == sv::log::llUndefined)
            throw SvException(QString(IMPERMISSIBLE_VALUE)
                               .arg(P).arg(QString(QJsonDocument(global).toJson(QJsonDocument::Compact)))
                               .arg(QString("Неверное значение для параметра \"%1\". Допустимые значения: error, warning, info, debug, debug2")
                                                  .arg(P_LOG_LEVEL)));
        }
        else
          p.log_level = sv::log::llInfo;
      }

  }

}
#endif // ZN_GLOBAL

