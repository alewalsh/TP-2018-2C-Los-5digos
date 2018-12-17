cd ..
git clone https://github.com/sisoputnfrba/so-commons-library
cd so-commons-library
sudo make install
cd ..
cd tp-2018-2c-Los-5digos
git checkout ramaFinal
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/utnso/git/tp-2018-2c-Los-5digos/GranTPCommons/Debug
ls -l /usr/include/commons && ls -l /usr/lib/
