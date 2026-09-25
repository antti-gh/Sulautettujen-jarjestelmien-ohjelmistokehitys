*** Settings ***
Library           SerialLibrary    encoding=ascii
Suite Setup       Valmistele Sarjaportti
Suite Teardown    Delete All Ports
 
*** Variables ***
${PORT}           COM12
 
*** Keywords ***
Valmistele Sarjaportti
    Add Port    ${PORT}    baudrate=115200
    Reset Input Buffer    ${PORT}
 
Testaa Komento
    [Arguments]    ${komento}    ${odotettu_vastaus}
    Reset Input Buffer    ${PORT}
    Write Data    ${komento}X
    ${vastaus}=    Read Until    X
    Should Contain    ${vastaus}    ${odotettu_vastaus}X
 
*** Test Cases ***
Testaa Oikea Aika
    Testaa Komento    000120    80
 
Testaa Virheellinen Aika
    Testaa Komento    006700    -3
 
Testaa Liian Lyhyt Aika
    Testaa Komento    123    -1
 
Testaa Nolla Aika
    Testaa Komento    000000    -4
 
Testaa Oikea Sekvenssi
    Testaa Komento    R,100    0
 
Testaa Sekvenssi Väärällä Värillä
    Testaa Komento    K,100    -11
 
Testaa Sekvenssi Nolla-ajalla
    Testaa Komento    R,0    -13