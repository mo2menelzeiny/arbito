# Arbito 

##### Dokku configurations
<p>Disable proxy <code>dokku proxy:disable arbito</code>
<p>Disable zero checks <code>dokku checks:disable arbito</code>
<p>Set network to host <code>dokku docker-options:add arbito deploy --network=host</code>
<p>Set shm size <code>dokku docker-options:add arbito deploy --shm-size="1g"</code>

##### CPU Isolation
<p>Ioslate cores 1,2,3,4 in grub configurations
<p>Edit grub file <code>vi /etc/default/grub</code>
<p>Add <code>"isolcpus=1,2,3,4"</code> to <code>GRUB_CMDLINE_LINUX_DEFAULT</code>
<p>Then <code>update-grub</code>
<p>Alternative is directly edeting the kernel entry in <code>/boot/grub/grub.conf</code>
<code>transparent_hugepage=madvise</code>
