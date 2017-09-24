# Chillbox Project 

### This is a project where I control my chill box with a pelletier effect chip.


#### How it works ?
A PID controlled by a Raspberry PI3 written in Python. I used a python module because it was quick to code.
I also use an arduino nano as a slave to provide other informations such as the cooling flow.

I guess the flow sensor could be used on the PI directly, but for the moment it's simpler like that. The arduino produces a "serial out" string and its
connected via USB to the Raspberry PI3.

The data is sent to a MySQL db locally hosted ( it could be hosted on the pi too, but I already had a server in the house ).

To close the loop and be able to gather/read all my data, I am working on a web page to display nice graph of what's happening in the chillbox

#### The Python code


#### The Arduino Code

