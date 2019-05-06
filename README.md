## mgba for rcore  
### We now compile them under alpine linux.  
### get libmgba.a  
```bash
# under mgba/
rm build -rf
mkdir build
cd build
cmake -DUSE_MINIZIP=OFF -DUSE_ZLIB=OFF -DUSE_LZMA=OFF ..
make
```
### get mgba.bin
```bash
# under rcore-port/
rm build -rf
mkdir build
cd build
cmake ..
make
mv mgba mgba.bin
```

