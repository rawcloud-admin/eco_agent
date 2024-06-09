1.INSTALL


Program is already compiled. You just need to install by running install.sh script:

./install.sh

After installation program will start running. You can check the status by running:

systemctl status eco_agent.service

2. UNINSTALL


If you want to uninstall the program run uninstall script:

./uninstall.sh


 
3.COMPILE

If you wnat to compile the program(there is already compiled, if neeeded), then you can run:

./compile


4.RUN

If the service is stopped you can run it again with follwoing commands:

systemctl start eco_agent.service
systemctl enable eco_agent.service


5.STOP

If you want to stop the program run following command:

./stop 
