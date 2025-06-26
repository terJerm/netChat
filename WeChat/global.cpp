#include "global.h"

///用来刷新 QWidget 的样式
std::function<void(QWidget*)> repolish = [](QWidget* w){
    w->setStyle(w->style());                        // 重新应用当前样式
};

///加密
std::function<QString(const QString& input,const QString& key,const QString& salt)> enhancedXorEncrypt =
    [](const QString& input,const QString& key,const QString& salt){
        QByteArray inputBytes = input.toUtf8();
        QByteArray keyBytes = (key + salt).toUtf8(); // 动态密钥
        QByteArray result;

        for (int i = 0; i < inputBytes.size(); ++i) {
            char encryptedChar = inputBytes[i] ^ keyBytes[i % keyBytes.size()];
            result.append(encryptedChar);
        }

        // 将盐加在密文前面，并Base64编码
        QByteArray final = salt.toUtf8() + result;
        return final.toBase64();
    };

///解密
std::function<QString(const QString& baseInput,const QString& key,int saltLength)> enhancedXorDecrypt =
    [](const QString& baseInput,const QString& key,int saltLength){
        QByteArray decoded = QByteArray::fromBase64(baseInput.toUtf8());
        QByteArray salt = decoded.left(saltLength);  // 从前面取出盐
        QByteArray encrypted = decoded.mid(saltLength);  // 剩下的是密文
        QByteArray keyBytes = (key + QString::fromUtf8(salt)).toUtf8();
        QByteArray result;

        for (int i = 0; i < encrypted.size(); ++i) {
            char decryptedChar = encrypted[i] ^ keyBytes[i % keyBytes.size()];
            result.append(decryptedChar);
        }

        return QString::fromUtf8(result);
    };


///生成指定长度范围内的随机字符串
QString generateRandomString(int minLength, int maxLength,const QString &charset ) {
    if (minLength > maxLength || minLength < 0) return QString();

    int length = QRandomGenerator::global()->bounded(minLength, maxLength + 1);
    QString result;

    for (int i = 0; i < length; ++i) {
        int index = QRandomGenerator::global()->bounded(0, charset.length());
        result.append(charset.at(index));
    }

    return result;
}
















QString gate_url_prefix = "";

QString salt = "s4h7B";

QString key = "MySeCuReKEy!";



SearchInfo::SearchInfo(int uid, QString name, QString nick,QString desc, int sex,QString icon)
    :_uid(uid),_name(name),_nick(nick),_desc(desc),_sex(sex),_icon(icon) {}

SearchInfo::SearchInfo() {}

QString formatFileSize(qint64 size)
{
    double num = size;
    QStringList units = {"B", "K", "M", "G", "T"};
    int unitIndex = 0;

    while (num >= 1024.0 && unitIndex < units.size() - 1) {
        num /= 1024.0;
        ++unitIndex;
    }

    /// 保留一位小数（小于 10 时），否则保留整数
    return (num < 10.0 ? QString::number(num, 'f', 1) : QString::number(num, 'f', 0)) + units[unitIndex];
}

QString getFileIconFromSuffix(QString suffix)
{
    QString iconPath;
    QString s = suffix.toLower();
    if (s == "7z" || s == "7zip" || s == "rar") {
        iconPath = ":/res/fileType/icons8-7zip-100.png";
    } else if (s == "ai") {
        iconPath = ":/res/fileType/icons8-ai-100.png";
    } else if (s == "avi") {
        iconPath = ":/res/fileType/icons8-avi-100.png";
    } else if (s == "csv") {
        iconPath = ":/res/fileType/icons8-csv-100.png";
    } else if (s == "dll") {
        iconPath = ":/res/fileType/icons8-dll-100.png";
    } else if (s == "doc" || s == "docx") {
        iconPath = ":/res/fileType/icons8-doc-100.png";
    } else if (s == "exe") {
        iconPath = ":/res/fileType/icons8-exe-100.png";
    } else if (s == "gif") {
        iconPath = ":/res/fileType/icons8-gif-100.png";
    } else if (s == "html" || s == "htm") {
        iconPath = ":/res/fileType/icons8-html-100.png";
    } else if (s == "jpg" || s == "jpeg") {
        iconPath = ":/res/fileType/icons8-jpg-100.png";
    } else if (s == "js") {
        iconPath = ":/res/fileType/icons8-js-100.png";
    } else if (s == "mov") {
        iconPath = ":/res/fileType/icons8-mov-100.png";
    } else if (s == "mp3") {
        iconPath = ":/res/fileType/icons8-mp3-100.png";
    } else if (s == "pdf") {
        iconPath = ":/res/fileType/icons8-pdf-2-100.png";
    } else if (s == "png") {
        iconPath = ":/res/fileType/icons8-png-100.png";
    } else if (s == "ppt" || s == "pptx") {
        iconPath = ":/res/fileType/icons8-ppt-100.png";
    } else if (s == "psd") {
        iconPath = ":/res/fileType/icons8-psd-100.png";
    } else if (s == "raw") {
        iconPath = ":/res/fileType/icons8-raw-100.png";
    } else if (s == "sql") {
        iconPath = ":/res/fileType/icons8-sql-100.png";
    } else if (s == "txt") {
        iconPath = ":/res/fileType/icons8-txt-100.png";
    } else if (s == "w") {
        iconPath = ":/res/fileType/icons8-word-100.png";
    } else if (s == "xls" || s == "xlsx") {
        iconPath = ":/res/fileType/icons8-xls-100.png";
    } else if (s == "xml") {
        iconPath = ":/res/fileType/icons8-xml-100.png";
    } else if (s == "zip") {
        iconPath = ":/res/fileType/icons8-zip-100.png";
    } else {
        iconPath = ":/res/fileType/icons8-unknown-100.png"; // 默认图标
    }
    return iconPath;
}
