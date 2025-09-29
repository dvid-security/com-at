# Flash com-at.bin


```md

**hold** BTLD Radio ~2-3sec then press cold restart  

pip install esptool

esptool.py --chip esp32c6 --port /dev/ttyUSB0 --baud 115200 write_flash \
  0x0 bootloader.bin \
  0x8000 partition-table.bin \
  0x10000 com-at.bin
```

If u want to monitor use screen or PuTy
