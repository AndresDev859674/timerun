<p align="center">
  <img style="width: 60%; height: auto;" alt="timerun" src="https://github.com/user-attachments/assets/0fe79cac-8f17-4b88-b714-3500705935c0" />
</p>
<p align="center"> Execute applications in a custom system time context </p>

<p align="center">
  <img src="https://theoldnet.com/images/anibar.gif">
</p>

# About
What its this? It's a program written in C that runs applications on a specific date without breaking the system by changing the system time. It communicates with the kernel. For example, if you want to program something that requires a specific date to change things (for events like Christmas, December 24th), you use TimeRun and can test if everything you programmed works (when it's activated on that date).

## Installation
```bash
git clone https://github.com/AndresDev859674/timerun.git
cd timerun
make
sudo make install
