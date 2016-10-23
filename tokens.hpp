#pragma once

inline bool is_reply(size_t numeric) {
  return numeric < 400;
}
inline bool is_error(size_t numeric) {
  return numeric >= 400;
}

#define ERR_NOSUCHNICK       401
#define ERR_NOSUCHCHANNEL    403
#define ERR_CANNOTSENDTOCHAN 404
#define ERR_TOOMANYCHANNELS  405

#define ERR_NOSUCHCMD        421

#define ERR_NONICKNAMEGIVEN  431
#define ERR_ERRONEUSNICKNAME 432
#define ERR_NICKNAMEINUSE    433

#define ERR_UNAVAILRESOURCE  437

#define ERR_USERNOTINCHANNEL 441
#define ERR_NOTONCHANNEL     442
#define ERR_USERONCHANNEL    443
#define ERR_NOLOGIN          444

#define ERR_NEEDMOREPARAMS   461
#define ERR_ALREADYREGISTRED 462
#define ERR_PASSWDMISMATCH   464
#define ERR_YOUREBANNEDCREEP 465
#define ERR_YOUWILLBEBANNED  466
#define ERR_KEYSET           467

#define ERR_CHANNELISFULL    471
#define ERR_UNKNOWNMODE      472
#define ERR_INVITEONLYCHAN   473
#define ERR_BANNEDFROMCHAN   474
#define ERR_BADCHANNELKEY    475
#define ERR_BADCHANMASK      476
#define ERR_NOCHANMODES      477
#define ERR_BANLISTFULL      478

#define ERR_NOPRIVILEGES      481
#define ERR_CHANOPRIVSNEEDED  482
#define ERR_UNIQOPPRIVSNEEDED 485

#define ERR_UMODEUNKNOWNFLAG 501
#define ERR_USERSDONTMATCH   502

#define RPL_WELCOME    001
#define RPL_YOURHOST   002
#define RPL_CREATED    003
#define RPL_MYINFO     004
#define RPL_CAPABS     005

#define RPL_USERHOST   302

#define RPL_WHOISUSER  311
#define RPL_WHOISIDLE  317
#define RPL_ENDOFWHOIS 318

#define RPL_ENDOFSTATS       219
#define RPL_STATSUPTIME      242
#define RPL_UMODEIS          221


#define RPL_LUSERCLIENT      251
#define RPL_LUSEROP          252
#define RPL_LUSERUNKNOWN     253
#define RPL_LUSERCHANNELS    254
#define RPL_LUSERME          255

#define RPL_ADMINME          256
#define RPL_ADMINLOC1        257
#define RPL_ADMINLOC2        258
#define RPL_ADMINEMAIL       259

#define RPL_UNIQOPIS         325
#define RPL_CHANNELMODEIS    324
#define RPL_CHANNELCREATED   329
#define RPL_NOTOPIC          331
#define RPL_TOPIC            332
#define RPL_TOPICBY          333

#define RPL_VERSION     351

#define RPL_NAMREPLY    353
#define RPL_ENDOFNAMES  366

#define RPL_INFO        371
#define RPL_ENDOFINFO   374

#define RPL_MOTDSTART   375
#define RPL_MOTD        372
#define RPL_ENDOFMOTD   376

#define RPL_TIME        391

#define RPL_USERSSTART  392
#define RPL_USERS       393
#define RPL_ENDOFUSERS  394


#define TK_CAP     "CAP"
#define TK_PASS    "PASS"
#define TK_NICK    "NICK"
#define TK_USER    "USER"

#define TK_MOTD     "MOTD"
#define TK_LUSERS   "LUSERS"
#define TK_USERHOST "USERHOST"
#define TK_STATS    "STATS"

#define TK_PING     "PING"
#define TK_PONG     "PONG"
#define TK_MODE     "MODE"
#define TK_JOIN     "JOIN"
#define TK_PART     "PART"
#define TK_TOPIC    "TOPIC"
#define TK_NAMES    "NAMES"
#define TK_PRIVMSG  "PRIVMSG"
#define TK_QUIT     "QUIT"

#define TK_CONNECT  "CONNECT"
#define TK_SQUIT    "SQUIT"
#define TK_WALLOPS  "WALLOPS"

#define TK_SVSNICK  "SVSNICK"
#define TK_SVSUSER  "SVSUSER"
#define TK_SVSHOST  "SVSHOST"
