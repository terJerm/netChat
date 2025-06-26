
const config_module = require('./config')
const Redis = require("ioredis");

//����redis �ͻ���ʵ��
const redisCli = new Redis({
    host: config_module.redis_host,
    port: config_module.redis_port,
    password: config_module.redis_passwd,
});


redisCli.on("error", function (err) {
    console.log("RedisCli connect error");
    RedisCli.quit();
});



/*
    ����key ���value
*/
async function GetRedis(key) {
    try {
        const result = await redisCli.get(key)
        if (result == null) {
            console.log('result:', '<' + result + '>', 'This key cannot be find...')
            return null
        }
        console.log('Result:', '<' + result + '>', 'Get key success!...');
        return result

    } catch (error) {
        console.log('GetRedis error is', error);
        return null
    }
}

/**
 * ����key��ѯredis���Ƿ����key
 * @param {*} key 
 * @returns 
 */
async function QueryRedis(key) {
    try {
        const result = await redisCli.exists(key)
        //  �жϸ�ֵ�Ƿ�Ϊ�� ���Ϊ�շ���null
        if (result === 0) {
            console.log('result:<', '<' + result + '>', 'This key is null...');
            return null
        }
        console.log('Result:', '<' + result + '>', 'With this value!...');
        return result
    } catch (error) {
        console.log('QueryRedis error is', error);
        return null
    }

}

/**
* ����key��value��������ʱ��
* @param {*} key 
* @param {*} value 
* @param {*} exptime 
* @returns 
*/
async function SetRedisExpire(key, value, exptime) {
    try {
        // ���ü���ֵ
        await redisCli.set(key, value)
        // ���ù���ʱ�䣨����Ϊ��λ��
        await redisCli.expire(key, exptime);
        return true;
    } catch (error) {
        console.log('SetRedisExpire error is', error);
        return false;
    }
}

/**
 * �˳�����
 */
function Quit() {
    redisCli.quit();
}

//�׳��ӿڸ��ⲿʹ��
module.exports = { GetRedis, QueryRedis, Quit, SetRedisExpire, }