# Arbito 
#### SBE 
##### Command to build SBE classes

<code>java -Dsbe.validation.xsd=/home/mo2menelzeiny/Projects/arbito/resources/sbe.xsd -Dsbe.target.language=cpp -Dsbe.java.generate.interfaces=true -Dsbe.generate.ir=true -jar sbe-all-1.8.2-SNAPSHOT.jar /home/mo2menelzeiny/Projects/arbito/resources/arbito_sbe.xml</code>
  
#### DOKKU
##### Dokku configurations
<p>Disable proxy <code>dokku proxy:disable arbito</code>
<p>Disable zero checks <code>dokku checks:disable arbito</code>
<p>Set deploy branch <code>dokku git:set arbtio deploy-branch development</code>
<p>Set network to host <code>dokku docker-options:add arbito deploy --network=host</code>
<p>Set shm size <code>dokku docker-options:add arbito deploy --shm-size="1g"</code>

##### Aeron configurations
<p>To configure on reboot add the entries in <code>vi /etc/sysctl.conf</code>
<p>Set TCP buffer size <code>sudo sysctl -w net.core.rmem_max=2097152</code>
<p>Set TCP buffer size <code>sudo sysctl -w net.core.wmem_max=2097152</code>
<p>Set timestamps off <code>sudo sysctl -w net.ipv4.tcp_timestamps=0</code>
<p>Set TCP to low latency <code>sudo sysctl -w net.ipv4.tcp_low_latency=1</code>

#### OS Scheduler
##### Grub boot
<p>Ioslate cores 1,2,3,4 in grub configurations
<p>Edit grub file <code>vi /etc/default/grub</code>
<p>Add <code>"isolcpus=1,2,3,4"</code> to <code>GRUB_CMDLINE_LINUX_DEFAULT</code>
<p>Then <code>update-grub</code>
<p>Alternative is directly edeting the kernel entry in <code>/boot/grub/grub.conf</code>
<code>transparent_hugepage=madvise</code>

##### CPUSet Shield
<p>Using cpuset tool $cset
<p>create a shield under the name of docker <code>cset shield -k on --userset=docker -c 1-4</code>

##### IRQ Balance
<p> stop <code>service irqbalance stop</code>
<p> remove from boot set <code>enabled="0"</code> in <code>/etc/default/irqbalance</code>
<p> or <code>update-rc.d -f irqbalance remove</code>

