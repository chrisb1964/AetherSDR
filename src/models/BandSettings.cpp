#include "BandSettings.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

namespace AetherSDR {

static QString bandSettingsFilePath()
{
    return QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)
           + "/AetherSDR/BandSettings.xml";
}

BandSettings::BandSettings(QObject* parent)
    : QObject(parent)
{
}

QString BandSettings::bandForFrequency(double freqMhz)
{
    for (int i = 0; i < kBandCount; ++i) {
        if (freqMhz >= kBands[i].lowMhz && freqMhz <= kBands[i].highMhz)
            return QString::fromLatin1(kBands[i].name);
    }
    return QStringLiteral("GEN");
}

const BandDef& BandSettings::bandDef(const QString& name)
{
    if (name == "WWV") return kWwvBand;
    for (int i = 0; i < kBandCount; ++i) {
        if (name == QLatin1String(kBands[i].name))
            return kBands[i];
    }
    return kGenBand;
}

void BandSettings::saveBandState(const QString& bandName, const BandSnapshot& snap)
{
    m_bandStates[bandName] = snap;
}

BandSnapshot BandSettings::loadBandState(const QString& bandName) const
{
    if (m_bandStates.contains(bandName))
        return m_bandStates[bandName];

    // Return defaults from band definition
    const auto& def = bandDef(bandName);
    BandSnapshot snap;
    snap.frequencyMhz    = def.defaultFreqMhz;
    snap.mode            = QString::fromLatin1(def.defaultMode);
    snap.minDbm          = -130.0f;
    snap.maxDbm          = -40.0f;
    snap.spectrumFrac    = 0.40f;
    return snap;
}

bool BandSettings::hasSavedState(const QString& bandName) const
{
    return m_bandStates.contains(bandName);
}

void BandSettings::saveToFile() const
{
    const QString path = bandSettingsFilePath();
    QDir().mkpath(QFileInfo(path).absolutePath());

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "BandSettings: failed to save" << path;
        return;
    }

    QXmlStreamWriter xml(&file);
    xml.setAutoFormatting(true);
    xml.writeStartDocument();
    xml.writeStartElement("BandSettings");

    for (auto it = m_bandStates.constBegin(); it != m_bandStates.constEnd(); ++it) {
        const BandSnapshot& s = it.value();
        xml.writeStartElement("Band");
        xml.writeAttribute("name", it.key());
        xml.writeTextElement("FrequencyMhz", QString::number(s.frequencyMhz, 'f', 6));
        xml.writeTextElement("Mode", s.mode);
        xml.writeTextElement("RxAntenna", s.rxAntenna);
        xml.writeTextElement("FilterLow", QString::number(s.filterLow));
        xml.writeTextElement("FilterHigh", QString::number(s.filterHigh));
        xml.writeTextElement("AgcMode", s.agcMode);
        xml.writeTextElement("AgcThreshold", QString::number(s.agcThreshold));
        xml.writeTextElement("RfGain", QString::number(s.rfGain));
        xml.writeTextElement("WnbOn", s.wnbOn ? "True" : "False");
        xml.writeTextElement("WnbLevel", QString::number(s.wnbLevel));
        xml.writeTextElement("MinDbm", QString::number(static_cast<double>(s.minDbm), 'f', 2));
        xml.writeTextElement("MaxDbm", QString::number(static_cast<double>(s.maxDbm), 'f', 2));
        xml.writeTextElement("SpectrumFrac", QString::number(static_cast<double>(s.spectrumFrac), 'f', 3));
        xml.writeEndElement(); // Band
    }

    xml.writeEndElement(); // BandSettings
    xml.writeEndDocument();
    qDebug() << "BandSettings: saved" << m_bandStates.size() << "bands to" << path;
}

void BandSettings::loadFromFile()
{
    const QString path = bandSettingsFilePath();
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;

    QXmlStreamReader xml(&file);
    while (!xml.atEnd()) {
        xml.readNext();
        if (!xml.isStartElement() || xml.name() != u"Band") continue;

        const QString bandName = xml.attributes().value("name").toString();
        if (bandName.isEmpty()) continue;

        BandSnapshot snap;
        while (!(xml.isEndElement() && xml.name() == u"Band") && !xml.atEnd()) {
            xml.readNext();
            if (!xml.isStartElement()) continue;
            const auto tag = xml.name();
            const QString val = xml.readElementText();
            if      (tag == u"FrequencyMhz") snap.frequencyMhz = val.toDouble();
            else if (tag == u"Mode")         snap.mode          = val;
            else if (tag == u"RxAntenna")    snap.rxAntenna     = val;
            else if (tag == u"FilterLow")    snap.filterLow     = val.toInt();
            else if (tag == u"FilterHigh")   snap.filterHigh    = val.toInt();
            else if (tag == u"AgcMode")      snap.agcMode       = val;
            else if (tag == u"AgcThreshold") snap.agcThreshold  = val.toInt();
            else if (tag == u"RfGain")       snap.rfGain        = val.toInt();
            else if (tag == u"WnbOn")        snap.wnbOn         = (val == "True");
            else if (tag == u"WnbLevel")     snap.wnbLevel      = val.toInt();
            else if (tag == u"MinDbm")       snap.minDbm        = val.toFloat();
            else if (tag == u"MaxDbm")       snap.maxDbm        = val.toFloat();
            else if (tag == u"SpectrumFrac") snap.spectrumFrac  = val.toFloat();
        }

        if (snap.isValid())
            m_bandStates[bandName] = snap;
    }
    qDebug() << "BandSettings: loaded" << m_bandStates.size() << "bands from" << path;
}

} // namespace AetherSDR
