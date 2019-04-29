## mgba for rcore  
----
### get libmgba.so  
```bash
# under mgba/
rm build -rf
mkdir build
cd build
cmake -DMUSL=/path/to/musl -DUSE_MINIZIP=OFF -DUSE_ZLIB=OFF -DUSE_LZMA=OFF ..
```

