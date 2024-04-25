
#Build FASTST

cd $ROOT/FASTST
mkdir -p build
cd build

if [[ "$FASTST_test" == "FASTST_test" ]]; then
	cmake -DCMAKE_INSTALL_PREFIX=./install ../ -FASTST_test=ON
else
  cmake -DCMAKE_INSTALL_PREFIX=./install ../
fi

cmake --build . --target install --parallel -j 40