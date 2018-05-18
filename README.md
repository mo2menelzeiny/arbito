# Arbito 
### FIX8 Build Configuration
1- Install Fix8 deps

2- Sometimes you need to incude '/usrlocal/lib/' in env path

<code>export LD_LIBRARY_PATH="/usr/local/lib:$LD_LIBRARY_PATH"</code>

3- Configure the build

<code>./configure --with-poco=/usr/local --enable-tbbmalloc=yes --with-mpmc=tbb --enable-rawmsgsupport=yes --enable-f8test=no --enable-ssl=yes --enable-fillmetadata=no --enable-doxygen-warnings=no</code>

NOTE: Disable doxygen for faster build time

4- f8c Compiler options

<code>f8c -I -p LMAX_FIX -n LMAX -c client -o /home/mo2menelzeiny/Desktop /home/mo2menelzeiny/Documents/HFT/LMAX-FIX-MarketData-API-QuickFix-DataDictionary.xml</code>
<code>f8c -I -p LMAX_FIX -n LMAX -c client -o /home/mo2menelzeiny/Desktop /home/mo2menelzeiny/Documents/HFT/LMAX-FIX-Trading-API-QuickFix-DataDictionary.xml</code>