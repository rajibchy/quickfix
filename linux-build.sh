# ubuntu 24.04 lts
# build test success 10:05 PM 1/1/2025
sudo apt update && sudo apt upgrade -y
sudo apt install -y build-essential cmake libssl-dev
sudo apt install -y build-essential cmake libssl-dev libpq-dev postgresql-16 postgresql-contrib

mkdir build
cd build


cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DQUICKFIX_TESTS=OFF -DQUICKFIX_EXAMPLES=OFF -DCMAKE_CXX_FLAGS="-O1" -DHAVE_SSL=ON -DHAVE_POSTGRESQL=ON -DCMAKE_INSTALL_PREFIX="/home/fsys/fsys/dependency/quickfix/linux-x64" ..
make -j4
make install

# Update the LD_LIBRARY_PATH for Dynamic Libraries
# If the QuickFIX library is a shared library (.so file), you'll need to ensure your system can find it by updating the LD_LIBRARY_PATH environment variable.
export LD_LIBRARY_PATH=/home/fsys/fsys/dependency/quickfix/lib/linux-x64:$LD_LIBRARY_PATH
source ~/.bashrc  # or `source ~/.zshrc` if you're using Zsh
# nstall to System Directories (Optional)
sudo cp /home/fsys/fsys/dependency/quickfix/linux-x64/lib/* /usr/local/lib/
sudo cp -r /home/fsys/fsys/dependency/quickfix/linux-x64/include/* /usr/local/include/

#Create the Symbolic Links for libquickfix.so
sudo ln -sf /usr/local/lib/libquickfix.so.16 /usr/local/lib/libquickfix.so

sudo ldconfig -p | grep quickfix
# must be display this result with version
# libquickfix.so.16 (libc6,x86-64) => /usr/local/lib/libquickfix.so.16
# libquickfix.so (libc6,x86-64) => /usr/local/lib/libquickfix.so
