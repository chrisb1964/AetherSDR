#include "BandSettings.h"

#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

namespace AetherSDR {

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
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)
                        + "/AetherSDR";
    QDir().mkpath(dir);
    const QString path = dir + "/BandStates.json";

    QJsonObject root;
    for (auto it = m_bandStates.constBegin(); it != m_bandStates.constEnd(); ++it) {
        const BandSnapshot& s = it.value();
        QJsonObject obj;
        obj["frequencyMhz"] = s.frequencyMhz;
        obj["mode"]         = s.mode;
        obj["rxAntenna"]    = s.rxAntenna;
        obj["txAntenna"]    = s.txAntenna;
        obj["filterLow"]    = s.filterLow;
        obj["filterHigh"]   = s.filterHigh;
        obj["agcMode"]      = s.agcMode;
        obj["agcThreshold"] = s.agcThreshold;
        obj["rfGain"]       = s.rfGain;
        obj["wnbOn"]        = s.wnbOn;
        obj["wnbLevel"]     = s.wnbLevel;
        obj["minDbm"]       = static_cast<double>(s.minDbm);
        obj["maxDbm"]       = static_cast<double>(s.maxDbm);
        obj["spectrumFrac"] = static_cast<double>(s.spectrumFrac);
        root[it.key()] = obj;
    }

    QFile file(path);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(root).toJson(QJsonDocument::Compact));
    } else {
        qWarning() << "BandSettings: failed to save" << path;
    }
}

void BandSettings::loadFromFile()
{
    const QString path = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)
                         + "/AetherSDR/BandStates.json";
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return;

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject()) return;

    const QJsonObject root = doc.object();
    for (auto it = root.constBegin(); it != root.constEnd(); ++it) {
        const QJsonObject obj = it.value().toObject();
        BandSnapshot s;
        s.frequencyMhz = obj["frequencyMhz"].toDouble();
        s.mode         = obj["mode"].toString();
        s.rxAntenna    = obj["rxAntenna"].toString();
        s.txAntenna    = obj["txAntenna"].toString();
        s.filterLow    = obj["filterLow"].toInt();
        s.filterHigh   = obj["filterHigh"].toInt();
        s.agcMode      = obj["agcMode"].toString();
        s.agcThreshold = obj["agcThreshold"].toInt();
        s.rfGain       = obj["rfGain"].toInt();
        s.wnbOn        = obj["wnbOn"].toBool();
        s.wnbLevel     = obj["wnbLevel"].toInt();
        s.minDbm       = static_cast<float>(obj["minDbm"].toDouble(-130.0));
        s.maxDbm       = static_cast<float>(obj["maxDbm"].toDouble(-40.0));
        s.spectrumFrac = static_cast<float>(obj["spectrumFrac"].toDouble(0.40));
        if (s.isValid())
            m_bandStates[it.key()] = s;
    }
    qDebug() << "BandSettings: loaded" << m_bandStates.size() << "band states from disk";
}

} // namespace AetherSDR
