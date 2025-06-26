
const grpc = require('@grpc/grpc-js')
const message_proto = require('./proto')
const const_module = require('./const')
const {v4: uuidv4} = require('uuid');
const emailModule = require('./email');
const redis_module = require('./redis')

///���10����֮�ڶ��������Ϊ��֤�뱻������redis�У����Իᱻ���÷��ظ��ͻ��ˡ�
async function GetVarifyCode(call, callback) {
    console.log("email is ", call.request.email)
    try {
        let query_res = await redis_module.GetRedis(const_module.code_prefix + call.request.email);
        console.log("query_res is ", query_res)
        if (query_res == null) {

        }
        let uniqueId = query_res;
        if (query_res == null) {
            uniqueId = uuidv4();
            if (uniqueId.length > 4) {
                uniqueId = uniqueId.substring(0, 4);
            }
            let bres = await redis_module.SetRedisExpire(const_module.code_prefix + call.request.email, uniqueId, 600)
            if (!bres) {
                callback(null, {
                    email: call.request.email,
                    error: const_module.Errors.RedisErr
                });
                return;
            }
        }

        console.log("uniqueId is ", uniqueId)

        let text_str = '������֤��Ϊ' + uniqueId + '����ʮ���������ע��'
        //�����ʼ�
        let mailOptions = {
            from: '3606204342@qq.com',
            to: call.request.email,
            subject: '��֤��',
            html: `<p>���ã�</p><p>������֤��Ϊ��<b style="font-size:16px">${uniqueId}</b></p><p>����ʮ���������ע�ᡣ</p>`,
            encoding: 'utf-8'
        };




        let send_res = await emailModule.SendMail(mailOptions);
        console.log("send res is ", send_res)

        callback(null, {
            email: call.request.email,
            error: const_module.Errors.Success
        });


    } catch (error) {
        console.log("catch error is ", error)

        callback(null, {
            email: call.request.email,
            error: const_module.Errors.Exception
        });
    }

}

function main() {
    const server = new grpc.Server();
    server.addService(message_proto.VarifyService.service, { GetVarifyCode });

    server.bindAsync('0.0.0.0:50051', grpc.ServerCredentials.createInsecure(), (err, port) => {
        if (err) {
            console.error('? gRPC ��ʧ��:', err.message);
            return;
        }
        console.log(`? gRPC server �Ѱ󶨶˿� ${port}`);
        server.start();
    });
}


main()


