cd ..
git clone https://github.com/sisoputnfrba/so-commons-library
cd so-commons-library
sudo make install
cd ..
cd tp-2018-2c-Los-5digos
git checkout ramaFinal
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/utnso/git/tp-2018-2c-Los-5digos/GranTPCommons/Debug
cd GranTPCommons/Debug
make clean && make all
sudo cp libGranTPCommons.so /usr/lib
cd ../../SAFA/Debug
make clean && make all
cd ../../MDJ_FileSystem/Debug
make clean && make all
cd ../../FunesMemory9/Debug
make clean && make all
cd ../../DMA/Debug
make clean && make all
cd ../../CPU/Debug
make clean && make all
