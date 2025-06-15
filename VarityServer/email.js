
const nodemailer = require('nodemailer');
const config_module = require("./config");

// ���� QQ ������ʼ��������
let transport = nodemailer.createTransport({
    host: 'smtp.qq.com',    // QQ ����� SMTP ������
    port: 465,              // QQ ����� SSL �˿�
    secure: true,           // ʹ�� SSL ����
    auth: {
        user: config_module.email_user, // ��� QQ ���䣨�磺123456@qq.com��
        pass: config_module.email_pass  // QQ �������Ȩ�루�������룡��
    }
});

/**
 * �����ʼ��ĺ���
 * @param {*} mailOptions_ �����ʼ��Ĳ���
 * @returns 
 */
function SendMail(mailOptions_){
    return new Promise(function(resolve, reject){
        transport.sendMail(mailOptions_, function(error, info){
            if (error) {
                console.log(error);
                reject(error);
            } else {
                console.log('�ʼ��ѳɹ����ͣ�' + info.response);
                resolve(info.response)
            }
        });
    })

}

module.exports.SendMail = SendMail