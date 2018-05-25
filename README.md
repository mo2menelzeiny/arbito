# Arbito 
### FIX8 Build Configuration
1- Install apt dependencies<br>
    
3- f8c build<br>
NOTE: Disable doxygen for faster build time
<code>export CXXFLAGS=-O3</code>
<code>./configure --prefix=/usr/local --with-mpmc=tbb --enable-tbbmalloc=yes --with-thread=stdthread --enable-preencode=yes --enable-rawmsgsupport=yes --enable-ssl=yes --enable-codectiming=yes --enable-fillmetadata=no --enable-f8test=no --enable-doxygen-warnings=no</code>

4- f8c Compiler options<br>
NOTE: Before running f8c rebuild library cache using <code>sudo ldconfig</code>
<code>f8c -p LMAX_FIXM -n LMAX_FIXM -c client -o /home/mo2menelzeiny/Desktop /home/mo2menelzeiny/Documents/HFT/LMAX-FIX-MarketData-API-QuickFix-DataDictionary.xml</code>
<code>f8c -p LMAX_FIX -n LMAX_FIX -c client -o /home/mo2menelzeiny/Desktop /home/mo2menelzeiny/Documents/HFT/LMAX-FIX-Trading-API-QuickFix-DataDictionary.xml</code>

5- Canon dependencies 
<code>conan install .. -s compiler=gcc -s compiler.libcxx=libstdc++11 --build=missing</code>