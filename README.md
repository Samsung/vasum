
                                 .lOX0c0k;           ,o00k;'
                                    'o0WMMW0o;.         ,doX0.
                                 .cxk',MMMMMMMMMWNNWWNX0x,:MMMk'
                   ':loxxxxdoodkNMMMMN oMMMMMMMMMMMMMMMMMMlkMMMMXd'     .
                ,OMMMMMMMMMMMMMMMMMMMM'.MMMMMMMMMMMMMMMMMW.OMMMMMMMWOo,  O0c
              .0W:kMMMMMMMMMMMMMMMMMMMl XMMMMMMMMMMMMMMMK.dMMMMMMMMMMMMW;l0;K:
              ::;c:lkWMMMMMMMMMMMMMMMMX oMMMMMMMMMMMMMMo.OMMMMMMMMMMMMM0,NMMMM.
               .kMWOl,;OMMMMMMMMMMMMMMMo.WMMMMMMMMMMMW,'NMMMMMMMMMMMM0,:WMMMMM'
              lWMMMMMMNd:xWMMMMMMMMMMMMM.:MMMMMMMMMMM;cMMMMMMMMMMMWx.;KMMMMMMM.
           .lNMMMMMMMMMMMO:lXMMMMMMMMMMM0 oMO;WMMMMModW;OMMMMMMM0c,oNMMMMMMMMMx    .
        'oKMMMMMMMMMMMMMMMMK:,xWMMMNWMMMMx .cl;''',c;;':xKXX0ddoxXMMMMMMMMMMMMM;   Xd
      'KMMMMMMMMMMMMMMMMMMMMMWd':OWckMX, 'c.:c;,,;ccdoK'      kMk;WMMMMMMMMMMMMW. 'NdO
     .WMMMMMMMMMMMMMMMMMMMMMMMMMKl:lko   ':;xNMMMMMMd0MMKxl.O: ,OK0kdl::cdkkkxdo:xWMMM,
     'N;0MMMWWWMMMMMMMMMMMMMMMMMMMMl     .OWKocxWMMMMlMMMMN,dOo   lWoXMMWNXXXNMMMMMMMW
      KXd,. .dlc:::cxKWMMMMMMMKKMMd  .:dKMMMMMMKloKk0cOxXk'kMMWo   ':lx0WMMMMMMMMMMMMc
            lMMMMMMWKkooolokKNkkMd ;kMMMMMMMMMMMMNodc  ,.'d,x0Ox'  xldl' .:dKMMMMMMMk
            OMMMMMMMMMMMMMNOxolo   Wl:.,kkxxxxkkko',              cMNMMMM0o;..;dKWNo .lc
           .MMMMMMMMMMMMMMMMMMMM.  ;   oMMMMMMMMMNKd            .'lWMMMMMMMMMN0kdlld0XdO
           0MMMMMMMMMMMMMMMMMMMMN.    :WMMMMMMMMMKld.         dNXdK,cXMMMMMMMMMMMMMMMMX'
          lMMMMMMMMMMMMMMMMMMMMMMN.  lMMMMMMXkoooxNMMKl:.xd0:,MMMMMMd.:XMMMMMMMMMMMWk,
         'MMMMMMMMMMMMMMMMMMMMMk;Nk  KMMM0xdxKWMMMMMMWd,XMNN: WMMMMMMNc :XMMMMMMMNc
         kKOMMMWKkdlc:;,''''''''''.  l0kd.XMMMMMMMMM0,lWMMMMl 0MMMMMMMMWd.:XMMMMk
         o0x0:.                       cO   xMMMMMMW:'XMMMMMMk OMMMMMMMMMMMKloxk;
          Od                                cMMMMMo KMMMMMMMW dMMMMMMMMMMMMMMXlOOl
                                             lMMMM0.,d0WMMMMM0.xOkdlccloxkOOOd::.
                                              ,kWMMKc:  ,dXMMMWd;.
                                                 .;:cxk;   .,co,ckk;
    ,clc,       cc:'
    lMMMMK     oMMMMo
     XMMMM,   .WMMMW.  ,lxOOOOxc.     .cdkOOkd;   :xxo.    cxxl.   xxd: ,dOOOxc .cxOOkd,
     :MMMMk   OMMMM; .NMMMX0NMMMMo   OMMMKOXMMMl  XMMMK    NMMMx  'MMMMWWKXMMMMXWN0WMMMM;
      KMMMW  ;MMMMl  .kXXl   NMMM0  'MMMX,  xxc   MMMMx   .MMMMo  cMMMMK.  XMMMMc  'MMMM:
      'MMMMc NMMMo    ;dOKNNWMMMMd   cXMMMMNOl.  .MMMM:   :MMMM;  xMMMM.   WMMMO   ;MMMM'
       kMMMXkMMMx   ;WMMMc. oMMMMc     .'c0MMMW. ;MMMM;  .NMMMM.  0MMMX   .MMMMc   dMMMM
       .WMMMMMMd    xMMMMX0NWMMMMMO.NMNOkkXMMMX  .WMMMMKKMWMMMW   XMMMk   :MMMM,   kMMMX
        .oxxxx:      cxO0Od; :k0Od. 'lxkO0Oko;    .ckO0kl..lxxo   'dxx,    lxxx    .oxxc


# Vasum
[Vasum](https://wiki.tizen.org/wiki/Security:Vasum) is a Linux daemon and a set of utilities used for managing para-virtualization. It uses Linux Containers to create separate, graphical environments called *zones*. One can concurrently run several zones on one physical device. Vasum exports a rich C/Dbus API that the application frameworks can use to interact with zones. 

For now Vasum uses [LXC](https://linuxcontainers.org/lxc/introduction/) for Linux Containers management. The project is mostly written in modern C++, is [well tested](https://wiki.tizen.org/wiki/Weekly_test_results_for_Tizen_3.X_security_framework).

Vasum's development takes place on [review.tizen.org/gerrit/](http://review.tizen.org/gerrit/) (registration on [tizen.org](http://tizen.org) is required).

## Installation and usage
The installation process and simple verification is described [here](https://wiki.tizen.org/wiki/Security:Vasum:Usage).

## Client interface
Vasum daemon can be accessed via C API or Dbus. You can find the API documentation [here](https://wiki.tizen.org/wiki/Security:Vasum:API). Be aware that the API will most likely change in the near future.

## Documentation
More comprehensive documentation is kept [here](https://wiki.tizen.org/wiki/Security:Vasum). You can generate the code documentation by executing *generate_documentation.sh* script from *doc* directory. Documentation will be generated in doc/html directory.

    cd ./doc
    ./generate_documentation.sh


## Code formatting
We use [astyle](http://astyle.sourceforge.net/) for code formatting (Use the latest version)
You can find the options file in the root of the project.

For example to format all .cpp and .hpp files run in the project directory:

    astyle --options=./astylerc --recursive ./*.cpp ./*.hpp