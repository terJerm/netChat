
const nodemailer = require('nodemailer');
const config_module = require("./config");

// 创建 QQ 邮箱的邮件传输代理
let transport = nodemailer.createTransport({
    host: 'smtp.qq.com',    // QQ 邮箱的 SMTP 服务器
    port: 465,              // QQ 邮箱的 SSL 端口
    secure: true,           // 使用 SSL 加密
    auth: {
        user: config_module.email_user, // 你的 QQ 邮箱（如：123456@qq.com）
        pass: config_module.email_pass  // QQ 邮箱的授权码（不是密码！）
    }
});

/**
 * 发送邮件的函数
 * @param {*} mailOptions_ 发送邮件的参数
 * @returns 
 */
function SendMail(mailOptions_){
    return new Promise(function(resolve, reject){
        transport.sendMail(mailOptions_, function(error, info){
            if (error) {
                console.log(error);
                reject(error);
            } else {
                console.log('邮件已成功发送：' + info.response);
                resolve(info.response)
            }
        });
    })

}

module.exports.SendMail = SendMail