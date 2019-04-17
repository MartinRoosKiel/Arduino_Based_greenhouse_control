# Arduino_Based_greenhouse_control
ethernet based control over light and temperatur in your greenhouse

To run it you will need a file named TembooAccount.h that contains:

#define TEMBOO_ACCOUNT "Your Temboo account name" 
#define TEMBOO_APP_KEY_NAME "Your Temboo app key name"
#define TEMBOO_APP_KEY "Your Temboo app key"

#define ETHERNET_SHIELD_MAC {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED}

#define PasswordValue = "temboo password"

#define FromEMailAddress = "fromAddress" // right now this serves also as username at my eMailprovider


#define ToEMailAddress = "toAddress"
 
