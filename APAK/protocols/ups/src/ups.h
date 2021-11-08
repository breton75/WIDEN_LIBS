﻿#ifndef UPS_H
#define UPS_H

#include <QMutex>
#include <QMutexLocker>

#include "ups_global.h"

#include "../../../../../../Modus/global/device/protocol/sv_abstract_protocol.h"
#include "../../../../../../Modus/global/signal/sv_signal.h"

#include "params.h"

#include "../../../../../../svlib/SvAbstractLogger/1.2/sv_abstract_logger.h"
#include "../../../../../../svlib/SvException/1.1/sv_exception.h"
#include "../../../../../../svlib/SvCRC/1.1/sv_crc.h"

extern "C" {

    UPS_EXPORT modus::SvAbstractProtocol* create();

//    VIRTUAL_DEVICESHARED_EXPORT QString defaultDeviceParams();
//    VIRTUAL_DEVICESHARED_EXPORT QString defaultIfcParams(const QString& ifc);
//    VIRTUAL_DEVICESHARED_EXPORT QList<QString> availableInterfaces();

}

namespace apak {

  struct UPSData {

    quint8  address;
    quint8  func;
    quint8  bytecnt;
    quint16 value;
    quint16 crc;

  };

  class SvUPS: public modus::SvAbstractProtocol
  {
    Q_OBJECT

  public:
    SvUPS();
    ~SvUPS();

    bool configure(modus::DeviceConfig* config, modus::IOBuffer *iobuffer) override;

    bool bindSignal(modus::SvSignal *signal, modus::SignalBinding binding) override;

  public slots:
    void start() override;

    void signalUpdated(modus::SvSignal* signal) override;
    void signalChanged(modus::SvSignal* signal) override;

  private:
    ups::ProtocolParams                       m_params;

    QMap<modus::SvSignal*, ups::SignalParams> m_input_signals;
    QMap<modus::SvSignal*, ups::SignalParams> m_params_by_signals;
    QMap<quint16, QList<modus::SvSignal*>> m_signals_by_registers;

    QQueue<quint16> send_queue;


  private slots:
    void queue();
    void request();
    void parse(modus::BUFF* buffer);

  signals:
    void nextRequest();

  };
}


#endif // UNIVERSAL_PACK_H
