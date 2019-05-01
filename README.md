## mgba for rcore  
### get libmgba.so  
```bash
# under mgba/
rm build -rf
mkdir build
cd build
export LD_LIBRARY_PATH=/path/to/musl/lib
cmake -DMUSL=/path/to/musl -DUSE_MINIZIP=OFF -DUSE_ZLIB=OFF -DUSE_LZMA=OFF ..
```

