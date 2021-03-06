extern "C" {
#include "dns.h"
}
#include "dnscrypt_client_helper.h"
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusMessage>
#include <private/qdbusmetatype_p.h>
#include <private/qdbusutil_p.h>
#include <QFile>
//#include <QTextStream>

#define UnitName QString("DNSCryptClient")

struct PrimitivePair
{
    QString         name;
    QVariant        value;
};
Q_DECLARE_METATYPE(PrimitivePair)
typedef QList<PrimitivePair>    SrvParameters;
Q_DECLARE_METATYPE(SrvParameters)

struct ComplexPair
{
    QString         name;
    SrvParameters   value;
};
Q_DECLARE_METATYPE(ComplexPair)
typedef QList<ComplexPair>      AuxParameters;
Q_DECLARE_METATYPE(AuxParameters)

/* unused
QString getRandomHex(const int &length)
{
    QString randomHex;
    for(int i = 0; i < length; i++) {
        int n = qrand() % 16;
        randomHex.append(QString::number(n,16));
    };
    return randomHex;
}
*/
QString getRespondIconName(qreal r)
{
    if        ( 0<r   && r<=0.3 ) {
        return "fast";
    } else if ( 0.3<r && r<=0.7 ) {
        return "middle";
    } else if ( 0.7<r && r<3.0 ) {
        return "slow";
    };
    return "none";
}
QString get_key_varmap(const QVariantMap &args, const QString& key)
{
    QString value;
    if (args.keys().contains(key)) {
        value = args[key].toString();
    };
    return value;
}
QString readFile(const QString &_path)
{
    QString ret;
    QFile f(_path);
    bool opened = f.open(QIODevice::ReadOnly);
    if ( opened ) {
        ret = QString::fromUtf8( f.readAll() );
        f.close();
    };
    return ret;
}
qint64  writeFile(const QString &_path, const QString &entry)
{
    qint64 ret = 0;
    QFile f(_path);
    bool opened = f.open(QIODevice::WriteOnly);
    if ( opened ) {
        ret = f.write(entry.toUtf8().data(), entry.size());
        f.close();
    } else {
        ret = -1;
    };
    return ret;
}

DNSCryptClientHelper::DNSCryptClientHelper(QObject *parent) :
    QObject(parent)
{
    qDBusRegisterMetaType<PrimitivePair>();
    qDBusRegisterMetaType<SrvParameters>();
    qDBusRegisterMetaType<ComplexPair>();
    qDBusRegisterMetaType<AuxParameters>();
}

QDBusArgument &operator<<(QDBusArgument &argument, const PrimitivePair &pair)
{
    argument.beginStructure();
    argument << pair.name << QDBusVariant(pair.value);
    argument.endStructure();
    return argument;
}
const QDBusArgument &operator>>(const QDBusArgument &argument, PrimitivePair &pair)
{
    argument.beginStructure();
    argument >> pair.name >> pair.value;
    argument.endStructure();
    return argument;
}
QDBusArgument &operator<<(QDBusArgument &argument, const SrvParameters &list)
{
    int id = qMetaTypeId<PrimitivePair>();
    argument.beginArray(id);
    //foreach (PrimitivePair item, list) {
    //    argument << item;
    //};
    SrvParameters::ConstIterator it = list.constBegin();
    SrvParameters::ConstIterator end = list.constEnd();
    for ( ; it != end; ++it)
        argument << *it;
    argument.endArray();
    return argument;
}
const QDBusArgument &operator>>(const QDBusArgument &argument, SrvParameters &list)
{
    argument.beginArray();
    while (!argument.atEnd()) {
        PrimitivePair item;
        argument >> item;
        list.append(item);
    };
    argument.endArray();
    return argument;
}

QDBusArgument &operator<<(QDBusArgument &argument, const ComplexPair &pair)
{
    argument.beginStructure();
    argument << pair.name << pair.value;
    argument.endStructure();
    return argument;
}
const QDBusArgument &operator>>(const QDBusArgument &argument, ComplexPair &pair)
{
    argument.beginStructure();
    argument >> pair.name >> pair.value;
    argument.endStructure();
    return argument;
}
QDBusArgument &operator<<(QDBusArgument &argument, const AuxParameters &list)
{
    int id = qMetaTypeId<ComplexPair>();
    argument.beginArray(id);
    //foreach (ComplexPair item, list) {
    //    argument << item;
    //};
    AuxParameters::ConstIterator it = list.constBegin();
    AuxParameters::ConstIterator end = list.constEnd();
    for ( ; it != end; ++it)
        argument << *it;
    argument.endArray();
    return argument;
}
const QDBusArgument &operator>>(const QDBusArgument &argument, AuxParameters &list)
{
    argument.beginArray();
    while (!argument.atEnd()) {
        ComplexPair item;
        argument >> item;
        list.append(item);
    };
    argument.endArray();
    return argument;
}

// not implemented; transient unit not started in system
// create transient DNSCryptClient.service systemd unit
/*
ActionReply DNSCryptClientHelper::create(const QVariantMap args) const
{
    ActionReply reply;

    const QString act = get_key_varmap(args, "action");
    if ( act!="create" ) {
        QVariantMap err;
        err["result"] = QString::number(-1);
        reply.setData(err);
        return reply;
    };
    QString servName =  get_key_varmap(args, "server");

    QDBusMessage msg = QDBusMessage::createMethodCall(
                "org.freedesktop.systemd1",
                "/org/freedesktop/systemd1",
                "org.freedesktop.systemd1.Manager",
                "StartTransientUnit");

    // Expecting 'ssa(sv)a(sa(sv))'
    // StartTransientUnit(in  s name,
    //                    in  s mode,
    //                    in  a(sv) properties,
    //                    in  a(sa(sv)) aux,
    //                    out o job);

    QString suffix = getRandomHex(8);
    QString name = QString("%1-%2.service")
            .arg(UnitName)
            .arg(suffix);           // service name + unique suffix
    QLatin1String mode("replace");  // If "replace" the call will start the unit
                                    // and its dependencies, possibly replacing
                                    // already queued jobs that conflict with this.

    SrvParameters _props;
    PrimitivePair srvType, execStart, execStop, user;
    srvType.name    = QLatin1String("Type");
    srvType.value   = QLatin1String("forking");
    execStart.name  = QLatin1String("ExecStart");
    execStart.value = QString("/sbin/dnscrypt-proxy -d -R %1").arg(servName);
    execStop.name   = QLatin1String("ExecStop");
    execStop.value  = QLatin1String("/bin/kill -INT ${MAINPID}");
    user.name       = QLatin1String("User");
    user.value      = QLatin1String("root");
    _props.append(srvType);
    _props.append(execStart);
    _props.append(execStop);
    _props.append(user);

    AuxParameters _aux;
    // aux is currently unused and should be passed as empty array.
    // ComplexPair   srvAuxData;
    //_aux.append(srvAuxData);

    QDBusArgument PROPS, AUX;
    PROPS   << _props;
    AUX     << _aux;

    QVariantList _args;
    _args   << name
            << mode
            << qVariantFromValue(PROPS)
            << qVariantFromValue(AUX);
    msg.setArguments(_args);
    QDBusMessage res = QDBusConnection::systemBus()
            .call(msg, QDBus::Block);
    QString str;
    foreach (QVariant arg, res.arguments()) {
        str.append(QDBusUtil::argumentToString(arg));
        str.append("\n");
    };

    QVariantMap retdata;
    retdata["msg"]          = str;
    retdata["name"]         = name;
    switch (res.type()) {
    case QDBusMessage::ReplyMessage:
        retdata["code"]     = QString::number(0);
        break;
    case QDBusMessage::ErrorMessage:
        retdata["code"]     = QString::number(1);
        retdata["err"]      = res.errorMessage();
        break;
    default:
        retdata["code"]     = QString::number(1);
        break;
    };

    reply.setData(retdata);
    return reply;
}
*/

ActionReply DNSCryptClientHelper::start(const QVariantMap args) const
{
    ActionReply reply;

    const QString act = get_key_varmap(args, "action");
    if ( act!="start" ) {
        QVariantMap err;
        err["result"] = QString::number(-1);
        reply.setData(err);
        return reply;
    };

    QString servName = get_key_varmap(args, "server");
    int jobPort      = get_key_varmap(args, "port").toInt();

    qint64 code = 0;
    QString entry = readFile("/etc/resolv.conf");
    if ( entry.startsWith("nameserver 127.0.0.1\nnameserver ::1\n") ) {
        entry.clear();
    } else {
        code = writeFile("/etc/resolv.conf", "nameserver 127.0.0.1\nnameserver ::1\n");
    };
    QVariantMap retdata;
    if ( code != -1 ) {
        QDBusMessage msg = QDBusMessage::createMethodCall(
                    "org.freedesktop.systemd1",
                    "/org/freedesktop/systemd1",
                    "org.freedesktop.systemd1.Manager",
                    "StartUnit");
        QList<QVariant> _args;
        _args<<QString("%1@%2.service")
               .arg(UnitName).arg(servName)
             <<"fail";
        msg.setArguments(_args);
        QDBusMessage res = QDBusConnection::systemBus()
                .call(msg, QDBus::Block);
        QString str;
        foreach (QVariant arg, res.arguments()) {
            str.append(QDBusUtil::argumentToString(arg));
            str.append("\n");
        };
        retdata["msg"]          = str;
        long unsigned int t     = 0;
        int domain = (servName.endsWith("ipv6"))? 6 : 4;
        switch (res.type()) {
        case QDBusMessage::ReplyMessage:
            retdata["entry"]    = entry;
            retdata["code"]     = QString::number(0);
            retdata["answ"]     = QString::number(
                        is_responsible(&t, jobPort, domain));
            retdata["resp"]     = getRespondIconName(qreal(t)/1000000);
            break;
        case QDBusMessage::ErrorMessage:
        default:
            retdata["entry"]    = entry;
            retdata["code"]     = QString::number(-1);
            retdata["err"]      = res.errorMessage();
            break;
        };
    } else {
        retdata["msg"]          = "Start failed";
        retdata["code"]         = QString::number(-1);
        retdata["err"]          = "Resolv.conf not changed";
    };

    reply.setData(retdata);
    return reply;
}

// unused; for stop and remove instantiated services used 'stopslice'
ActionReply DNSCryptClientHelper::stop(const QVariantMap args) const
{
    ActionReply reply;

    const QString act = get_key_varmap(args, "action");
    if ( act!="stop" ) {
        QVariantMap err;
        err["result"] = QString::number(-1);
        reply.setData(err);
        return reply;
    };

    QString servName =  get_key_varmap(args, "server");

    QVariantMap retdata;
    QDBusMessage msg = QDBusMessage::createMethodCall(
                "org.freedesktop.systemd1",
                "/org/freedesktop/systemd1",
                "org.freedesktop.systemd1.Manager",
                "StopUnit");
    QList<QVariant> _args;
    _args<<QString("%1@%2.service")
           .arg(UnitName).arg(servName)
         <<"fail";
    msg.setArguments(_args);
    QDBusMessage res = QDBusConnection::systemBus()
            .call(msg, QDBus::Block);
    QString str;
    foreach (QVariant arg, res.arguments()) {
        str.append(QDBusUtil::argumentToString(arg));
        str.append("\n");
    };
    retdata["msg"]          = str;
    switch (res.type()) {
    case QDBusMessage::ReplyMessage:
        retdata["code"]     = QString::number(0);
        break;
    case QDBusMessage::ErrorMessage:
        retdata["code"]     = QString::number(-1);
        retdata["err"]      = res.errorMessage();
        break;
    default:
        retdata["msg"]      = "Stop failed";
        retdata["code"]     = QString::number(-1);
        break;
    };

    reply.setData(retdata);
    return reply;
}

ActionReply DNSCryptClientHelper::restore(const QVariantMap args) const
{
    ActionReply reply;

    const QString act = get_key_varmap(args, "action");
    if ( act!="restore" ) {
        QVariantMap err;
        err["result"] = QString::number(-1);
        reply.setData(err);
        return reply;
    };

    qint64 code = -1;
    QString entry = args["entry"].toString();
    code = writeFile("/etc/resolv.conf", entry);
    QVariantMap retdata;
    if ( code != -1 ) {
        retdata["msg"]      = "Restore down";
    } else {
        retdata["msg"]      = "Restore failed";
    };
    retdata["code"]         = QString::number(code);

    reply.setData(retdata);
    return reply;
}

ActionReply DNSCryptClientHelper::stopslice(const QVariantMap args) const
{
    ActionReply reply;

    const QString act = get_key_varmap(args, "action");
    if ( act!="stopslice" ) {
        QVariantMap err;
        err["result"] = QString::number(-1);
        reply.setData(err);
        return reply;
    };

    QVariantMap retdata;
    QDBusMessage msg = QDBusMessage::createMethodCall(
                "org.freedesktop.systemd1",
                "/org/freedesktop/systemd1",
                "org.freedesktop.systemd1.Manager",
                "StopUnit");
    QList<QVariant> _args;
    _args<<QString("system-DNSCryptClient.slice")<<"fail";
    msg.setArguments(_args);
    QDBusMessage res = QDBusConnection::systemBus()
            .call(msg, QDBus::Block);
    QString str;
    foreach (QVariant arg, res.arguments()) {
        str.append(QDBusUtil::argumentToString(arg));
        str.append("\n");
    };
    retdata["msg"]          = str;
    switch (res.type()) {
    case QDBusMessage::ReplyMessage:
        retdata["code"]     = QString::number(0);
        break;
    case QDBusMessage::ErrorMessage:
        retdata["code"]     = QString::number(-1);
        retdata["err"]      = res.errorMessage();
        break;
    default:
        retdata["msg"]      = "Slice not stopped";
        retdata["code"]     = QString::number(-1);
        break;
    };

    reply.setData(retdata);
    return reply;
}

ActionReply DNSCryptClientHelper::startv2(const QVariantMap args) const
{
    ActionReply reply;

    const QString act = get_key_varmap(args, "action");
    if ( act!="start" ) {
        QVariantMap err;
        err["result"] = QString::number(-1);
        reply.setData(err);
        return reply;
    };

    QDBusMessage msg = QDBusMessage::createMethodCall(
                "org.freedesktop.systemd1",
                "/org/freedesktop/systemd1",
                "org.freedesktop.systemd1.Manager",
                "StopUnit");
    QList<QVariant> _args;
    _args<<"dnscrypt-proxy.service"<<"fail";
    msg.setArguments(_args);
    QDBusMessage res = QDBusConnection::systemBus()
            .call(msg, QDBus::Block);

    const QString servName = get_key_varmap(args, "server");
    qint64 code = 0;
    QString entry = readFile("/etc/dnscrypt-proxy/dnscrypt-proxy.toml");
    QStringList _data, _new_data;
    _data = entry.split("\n");
    foreach(QString _s, _data) {
        if ( _s.startsWith("server_names") ) {
            QStringList _parts = _s.split(" = ");
            _s.clear();
            _s.append(_parts.first());
            _s.append(" = ");
            _s.append(QString("['%1']").arg(servName));
        };
        _new_data.append(_s);
    };
    code = writeFile("/etc/dnscrypt-proxy/dnscrypt-proxy.toml", _new_data.join("\n"));
    entry = readFile("/etc/resolv.conf");
    code = 0;
    if ( entry.startsWith("nameserver 127.0.0.1\nnameserver ::1\n") ) {
        entry.clear();
    } else {
        code = writeFile("/etc/resolv.conf", "nameserver 127.0.0.1\nnameserver ::1\n");
    };

    QVariantMap retdata;
    msg = QDBusMessage::createMethodCall(
                "org.freedesktop.systemd1",
                "/org/freedesktop/systemd1",
                "org.freedesktop.systemd1.Manager",
                "StartUnit");
    //QList<QVariant> _args;
    _args.clear();
    _args<<"dnscrypt-proxy.service"<<"fail";
    msg.setArguments(_args);
    res = QDBusConnection::systemBus()
            .call(msg, QDBus::Block);
    QString str;
    foreach (QVariant arg, res.arguments()) {
        str.append(QDBusUtil::argumentToString(arg));
        str.append("\n");
    };

    retdata["msg"]          = str;
    long unsigned int t     = 0;
    int domain = (servName.endsWith("ipv6") || servName.contains("ip6")) ? 6 : 4;
    switch (res.type()) {
    case QDBusMessage::ReplyMessage:
        retdata["entry"]    = entry;
        retdata["code"]     = QString::number(0);
        retdata["answ"]     = QString::number(
                    is_responsible(&t, 53, domain)); // used default DNS port
        retdata["resp"]     = getRespondIconName(qreal(t)/1000000);
        break;
    case QDBusMessage::ErrorMessage:
    default:
        retdata["entry"]    = entry;
        retdata["code"]     = QString::number(-1);
        retdata["err"]      = res.errorMessage();
        break;
    };

    reply.setData(retdata);
    return reply;
}

ActionReply DNSCryptClientHelper::stopv2(const QVariantMap args) const
{
    ActionReply reply;

    const QString act = get_key_varmap(args, "action");
    if ( act!="stop" ) {
        QVariantMap err;
        err["result"] = QString::number(-1);
        reply.setData(err);
        return reply;
    };

    QVariantMap retdata;
    QDBusMessage msg = QDBusMessage::createMethodCall(
                "org.freedesktop.systemd1",
                "/org/freedesktop/systemd1",
                "org.freedesktop.systemd1.Manager",
                "StopUnit");
    QList<QVariant> _args;
    _args<<"dnscrypt-proxy.service"<<"fail";
    msg.setArguments(_args);
    QDBusMessage res = QDBusConnection::systemBus()
            .call(msg, QDBus::Block);
    QString str;
    foreach (QVariant arg, res.arguments()) {
        str.append(QDBusUtil::argumentToString(arg));
        str.append("\n");
    };
    retdata["msg"]          = str;
    switch (res.type()) {
    case QDBusMessage::ReplyMessage:
        retdata["code"]     = QString::number(0);
        break;
    case QDBusMessage::ErrorMessage:
        retdata["code"]     = QString::number(-1);
        retdata["err"]      = res.errorMessage();
        break;
    default:
        retdata["msg"]      = "Stop failed";
        retdata["code"]     = QString::number(-1);
        break;
    };

    reply.setData(retdata);
    return reply;
}

KAUTH_HELPER_MAIN("pro.russianfedora.dnscryptclient", DNSCryptClientHelper)
