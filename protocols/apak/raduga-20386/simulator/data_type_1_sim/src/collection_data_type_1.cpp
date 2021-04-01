﻿#include "collection_data_type_1.h"

raduga::DataType1::DataType1():
  SvAbstractSignalCollection()
{

}

void raduga::DataType1::addSignal(modus::SvSignal* signal) //throw (SvException)
{
  try
  {
    opa::SignalParams_0x04 p = opa::SignalParams_0x04::fromJson(signal->config()->params);

    m_signals.append(Signal0x04(signal, p));

  }
  catch(SvException& e)
  {
    throw SvException(QString("Сигнал %1: %2").arg(signal->config()->name, e.error));
  }
}

void opa::Type0x04::updateSignals(const opa::DATA* data)
{
  if(!data)
    return;

  foreach (Signal0x04 signal04, m_signals) {

    if(signal04.params.byte < data->len)
      signal04.signal->setValue(int((data->data[signal04.params.byte] >> signal04.params.bit) & 1));

  }
}
