# gsm-power-mon

A simple voltage monitor running on an esp32 and pushing data to elasticsearch over gprs + ssl

Sample collect log:
```
ets Jun  8 2016 00:22:57

rst:0x5 (DEEPSLEEP_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)
configsip: 0, SPIWP:0xee
clk_drv:0x00,q_drv:0x00,d_drv:0x00,cs0_drv:0x00,hd_drv:0x00,wp_drv:0x00
mode:DIO, clock div:2
load:0x3fff0018,len:4
load:0x3fff001c,len:1044
load:0x40078000,len:8896
load:0x40080400,len:5828
entry 0x400806ac
2020-12-31 16:33:00 Reset cause: ESP_RST_DEEPSLEEP
2020-12-31 16:33:00 Wake up cause: ESP_SLEEP_WAKEUP_TIMER
2020-12-31 16:33:00 Compile time 2020-12-31 15:22:51
2020-12-31 16:33:00 Booting in 2
2020-12-31 16:33:01 Booting in 1
2020-12-31 16:33:02 Booting in 0
2020-12-31 16:33:03 PMU percent 100
2020-12-31 16:33:03 Voltage data 6: 100, 2672, 12.58
2020-12-31 16:33:03 Next collect 2020-12-31 16:34:00
2020-12-31 16:33:06 Deep sleep until 2020-12-31 16:34:00
```

Sample push log:
```
2020-12-31 16:25:06 Deep sleep until 2020-12-31 16:25:52
ets Jun  8 2016 00:22:57

rst:0x5 (DEEPSLEEP_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)
configsip: 0, SPIWP:0xee
clk_drv:0x00,q_drv:0x00,d_drv:0x00,cs0_drv:0x00,hd_drv:0x00,wp_drv:0x00
mode:DIO, clock div:2
load:0x3fff0018,len:4
load:0x3fff001c,len:1044
load:0x40078000,len:8896
load:0x40080400,len:5828
entry 0x400806ac
2020-12-31 16:25:52 Reset cause: ESP_RST_DEEPSLEEP
2020-12-31 16:25:52 Wake up cause: ESP_SLEEP_WAKEUP_TIMER
2020-12-31 16:25:52 Compile time 2020-12-31 15:22:51
2020-12-31 16:25:52 Booting in 2
2020-12-31 16:25:53 Booting in 1
2020-12-31 16:25:54 Booting in 0
2020-12-31 16:25:55 PMU percent 100
2020-12-31 16:25:57 Initializing modem... OK
2020-12-31 16:26:04 Turning off SIM800 Red LED...
2020-12-31 16:26:04 Modem: SIM800 R14.18
2020-12-31 16:26:04 Waiting for network... OK
2020-12-31 16:26:09 Network connected
Modem time: 2020-12-31 17:26:04
2020-12-31 16:26:04 Reinitialized clock
2020-12-31 16:26:04 Connecting to APN: internet fail
2020-12-31 16:28:40 Light sleep until 2020-12-31 16:28:50
2020-12-31 16:28:48 PMU percent 100
2020-12-31 16:28:48 Network connected
2020-12-31 16:28:48 Signal quality20
Modem time: 2020-12-31 17:28:50
2020-12-31 16:28:50 Reinitialized clock
2020-12-31 16:28:50 Voltage data 29: 100, 2671, 12.58
2020-12-31 16:28:50 Next collect 2020-12-31 16:27:00
2020-12-31 16:28:50 Connecting to APN: internet OK
2020-12-31 16:30:24 Sending 30 voltage data
2020-12-31 16:30:24 Posting data:
{"timestamp":1609430497000,"internalPercent":100,"external":12.59471,"externalRaw":2674}

2020-12-31 16:30:34 Got status code: 201
Content length is: 179

Body returned follows:
{"_index":"mc-voltage","_type":"_doc","_id":"hTSiuXYBdwcnUz8j-Tw2","_version":1,"result":"created","_shards":{"total":2,"successful":2,"failed":0},"_seq_}
```
