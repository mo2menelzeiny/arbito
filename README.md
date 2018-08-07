# Arbito 
#### SBE 
##### Command to build SBE classes

<code>java -jar sbe-all-1.8.2-SNAPSHOT.jar -Dsbe.validation.xsd=/home/mo2menelzeiny/Projects/arbito/resources/sbe.xsd -Dsbe.target.language=cpp -Dsbe.java.generate.interfaces=true -Dsbe.generate.ir=true /home/mo2menelzeiny/Projects/arbito/resources/arbito_sbe.xml</code>
  
#### DOKKU
##### Dokku configurations
<p>Disable proxy <code>dokku proxy:disable arbitp</code>
<p>Disable zero checks <code>dokku checks:disable arbito</code>
<p>Set deploy branch <code>dokku git:set arbtio deploy_branch development</code>
<p>Set network to host <code>dokku docker-options:add arbito deploy --network=host</code>
