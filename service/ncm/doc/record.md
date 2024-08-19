## 播放数据回传

### /openapi/music/basic/play/data/record


### 请求方式：

- POST

### 公共参数：

[IOT公共参数](https://developer.music.163.com/st/developer/?docId=0f7801d7d6d24180b8fc9058d1ffe593)

[SDK公共参数](https://developer.music.163.com/st/developer/?docId=eb75c66cac074beda81216669a4192c9)

[WEB公共参数](https://developer.music.163.com/st/developer/?docId=72801c136f144658995fe1ab756a183e)

[移动端公共参数](https://developer.music.163.com/st/developer/?docId=98b78be8870c4bfe85aec7791c167f0a)

**注意: 公共参数中的accessToken需保证每台设备或每个用户唯一。如匿名登录时，每台设备需生成唯一的匿名accessToken。**

- 一旦有实名登录，就需要切实名token进行回传

### 业务参数（bizContent）：

**开始播放的数据回传**

| 参数名 | 必选  | 类型  | 说明  |
| :--- | :--- | :--- | --- |
| action | 是   | String | 行为，startplay:开始播放 |
| bitrate | 是   | Int | 码率，单位/kbps。song传实际码率，杜比固定传2999，dj固定传128 |
| file | 是   | Int | 文件类型回传枚举值：0：下载文件， 1：本地文件， 2：缓存文件， 3：云盘文件， 4：线上文件 |
| type | 是   | Sting | 类型，song:歌曲, dj:播客 |
| startLogTime | 是   | long | 开始播放时的时间戳，精确到毫秒 |
| id  | 是   | Sting | 当前资源id, 例如：歌曲的id，声音的真实id或者encVoiceId |
| alg | 是   | Sting | 算法使用，私人漫游、每日推荐和场景电台已返回该字段，产生播放时需透传alg，其余场景可参考下方 |
| isAudition | 否   | Int | 试听类型，1-片段试听（固定传1），2-全曲试听 |
| auditionStart | 否   | Int | 试听片段开始时间偏移（从上游接口取），单位/s |
| auditionEnd | 否   | Int | 试听片段时间偏移（从上游接口取），单位/s |

**结束播放的数据回传**

| 参数名 | 必选  | 类型  | 说明  |
| :--- | :--- | :--- | --- |
| action | 是   | String | 行为，结束播放：play |
| bitrate | 是   | Int | 码率，单位/kbps。song传实际码率（返回的br字段），杜比固定传2999，dj固定传128 |
| file | 是   | Int | 文件类型回传枚举值：0：下载文件， 1：本地文件， 2：缓存文件， 3：云盘文件， 4：线上文件 |
| type | 是   | Sting | 类型，song:歌曲, dj:播客 |
| startLogTime | 是   | long | 开始播放时的时间戳，精确到毫秒 |
| id  | 是   | Sting | 当前资源id, 例如：歌曲的id，声音的真实id或者encVoiceId |
| time | 是   | double | 播放时长，单位：秒， 播放完成或者中止播放时的时长，按照实际播放时长统计 |
| end | 是   | Sting | 结束方式回传枚举值：playend：正常结束；interrupt：第三方APP打断： exception: 错误； ui: 用户切歌 |
| alg | 是   | Sting | 算法使用，私人漫游、每日推荐和场景电台已返回该字段，产生播放时需透传alg，其余场景可参考下方 |
| isAudition | 否   | Int | 试听类型，1-片段试听（固定传1），2-全曲试听 |
| auditionStart | 否   | Int | 试听片段开始时间偏移（从上游接口取），单位/s |
| auditionEnd | 否   | Int | 试听片段时间偏移（从上游接口取），单位/s |
| sourceId | 否   | Sting | 播放来源资源ID（最近常听必传） |
| sourceType | 否   | Sting | 播放来源资源类型（最近常听必传） |

- auditionStart、auditionEnd就是试听起止时间，用作版权结算，不是用户真实播放时长
- alg字段除了每日推荐、私人漫游、场景电台等场景分发外，需要增加上游数据来源，回传歌单id、艺人id、专辑id等

| 序号  | 场景  | alg |
| :--- | :--- | --- |
| 1   | 每日推荐 | 接口分发 |
| 2   | 私人漫游 | 接口分发 |
| 3   | 场景电台 | 接口分发 |
| 4   | 搜索  | search |
| 5   | 杜比专区 | dobly_上游资源id |
| 6   | hires专区 | hires_上游资源id |

**action字段详解**

| action 名字 | 含义  | 回传时机 |
| :--- | :--- | --- |
| startplay | 播放开始 | 播放开始打点，不包含暂停-->开始 |
| play | 播放结束 | 播放结束打点，不包含暂停 |

- 综上，一首歌的回传日志有且仅有两条，startplay和play

**time字段详解**

| 序号  | 场景  | time |
| :--- | :--- | --- |
| 1   | 用户正常听歌10s | 10  |
| 2   | ⽤户正常听歌10S，2倍速听歌10S | 10+2\*10 = 30 |
| 3   | ⽤户正常听歌10S，2倍速听歌10S，0.5倍速听10S | 10+2\* 10+0.5\* 10= 35 |
| 4   | ⽤户正常听歌10S后滑动到歌曲的第20S继续听10S 到歌曲的第30S | 10+10=20 |
| 5   | ⽤户正常听歌30S后滑动到歌曲的第10S继续听20S 到歌曲的第30S | 30+20=50 |
| 6   | 其他滑动和倍速交叉场景⼀致 |     |

- 倍速场景下的歌曲播放时⻓ ：time=（time1 \* speed1+time2 \* speed2···）,无倍速时和time一致

**sourceType**

| type | 资源  |
| :--- | :--- |
| list | 歌单  |
| album | 专辑  |
| dailySongRecommend | 每日推荐 |
| userfm | 私人漫游 |
| djradio | 播单  |

**sourceId**

| 条件  | 值   |
| :--- | :--- |
| 播放来源为歌单，sourceType=list时 | sourceId=来源歌单ID |
| 播放来源为专辑，sourceType=album时 | sourceId=来源专辑ID |
| 播放来源为每日推荐，sourceType=dailySongRecommend时 | sourceId=dailySongRecommend |
| 播放来源为私人漫游，sourceType=userfm时 | sourceId=userfm |
| 播放来源为播单，sourceType=djradio时 | sourceId=来源播单ID |

### 请求事例：

```
https://openapi.music.163.com/openapi/music/basic/play/data/record?appId=a301020000000000746f96a196e52e07&signType=RSA_SHA256&timestamp=1660881438305&device={"deviceType":"andrcar","os":"andrcar","appVer":"0.1","channel":"didi","model":"kys","deviceId":"357","brand":"didi","osVer":"8.1.0","clientIp":"192.168.0.1"}&bizContent={"action":"startplay","bitrate":128,"file":4,"type":"song","startLogTime":"1653373289000","id":"01AF719CAD443AA5944BFE83F8F3569A"}&sign=hoxD0q9CyLZj%2BZ2yYLjCVs1sVInjphTFecgzWc6Y3zIOL1GTidmwXxc7Yv6HNOsf%2B9rKUT3mEi%2BkDhj22yqrBrnDEVMjySSAM21OSzESHL0RdtS2aLaTG7kmJmK4VX0S6UliNE57gOLJSWxICv00eT%2Bnbss8gGUREvkgo867%2B1qczMhqa5HN0v6HNJbaa5NSUmJX9lUBRBIlwpa9qIVLnYtbG7oWS0TbWbku%2FLluQDzBxyuBhv0BXZKCjCVZ9D3xL8Tegldxq%2B%2FMftaudJjD0eA8nR9d%2BzHEVvmrTZE7LXSeHrT0iJcz6goz%2BSHPjFtY0HChPaGV%2FOYrAnSF6r2h0Q%3D%3D&accessToken=sbc3d8e0323707d8fe8858f3c1561734547ad71544abbef80c
```

### 返回示例

```
{
    "code":200,
    "subCode":null,
    "message":null,
    "data":true / false
}
```
