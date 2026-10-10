#include "rtc.hpp"

//µDéclarations de fonctions utilisés uinuqmenet dans ce fichier :
static bool bornes(u8 minimal, u8 date, i8& changement, i8 limite);

static void changerAnnee(RtcDateTime& date, i8 changement);
static void changerMois(RtcDateTime& date, i8 changement);
static void changerJour(RtcDateTime& date, i8 changement);

static void changerHeure(RtcDateTime& date, i8 changement);
static void changerMinute(RtcDateTime& date, i8 changement);
static void changerSeconde(RtcDateTime& date, i8 changement);



// Initalisation RTC
ThreeWire myWire(PIN_IO,PIN_CLK,PIN_CE);
RtcDS1302 <ThreeWire> Rtc(myWire);
static RtcDateTime now;
Encodeur temp;
// Variables de texte a afficher
static char textDate[11];
static char textHeure[9];

void initHeure(){
    
    Rtc.Begin();
    Rtc.SetIsWriteProtected(false);
    if (!Rtc.IsDateTimeValid()) {
        Rtc.SetDateTime(RtcDateTime(__DATE__, __TIME__));
    }
    if (!Rtc.GetIsRunning()) Rtc.SetIsRunning(true);
    now = Rtc.GetDateTime();
}


RtcDateTime modifierDate(){
    RtcDateTime nouvelleDate = Rtc.GetDateTime();
    i8 changement = 0;
    i8 sortie = 0;
    while (sortie<3){
//A toi de jouer bastien ici j'ai besoin de la fonction suivante pour afficher les modifications constantes de la date 
// :  AfficherDate()
        temp.loopEncodeur();
        if(temp.aGauche()){
            changement--;
        }
        else if(temp.aDroite()){
            changement++;
        }
        else if(temp.aAppui()){
            sortie++;
        }
        if (changement!=0){
            switch(sortie){
                case 0 :
                    changerAnnee(nouvelleDate,changement);
                break;
                case 1 : 
                    changerMois(nouvelleDate,changement);
                    break;
                case 2 :
                    changerJour(nouvelleDate,changement);
                    break;
            }
        }
        changement = 0;
    }
    return nouvelleDate;

}

RtcDateTime modifierHeure(){
    RtcDateTime nouvelleDate = Rtc.GetDateTime();
    i8 changement = 0;
    i8 sortie = 0;
    while (sortie<3){
//A toi de jouer bastien ici j'ai besoin de la fonction suivante pour afficher les modifications constantes de l(heure) 
// :  AfficherDate()
        temp.loopEncodeur();
        if(temp.aGauche()){
            changement--;
        }
        else if(temp.aDroite()){
            changement++;
        }
        else if(temp.aAppui()){
            sortie++;
        }
        if (changement!=0){
            switch(sortie){
                case 0 :
                    changerHeure(nouvelleDate,changement);
                    break;
                case 1 : 
                    changerMinute(nouvelleDate,changement);
                    break;
                case 2 :
                    changerSeconde(nouvelleDate,changement);
                    break;
            }
        }
        changement = 0;
    }
    return nouvelleDate;

}

void texteDateHeure(){

    snprintf(textDate,sizeof(textDate),"%02u:%02u:%04u",
             (unsigned int)now.Day(),
             (unsigned int)now.Month(),
             (unsigned int)now.Year());
    
    snprintf(textHeure,sizeof(textHeure),"%02u:%02u:%02u",
             (unsigned int)now.Hour(),
             (unsigned int)now.Minute(),
             (unsigned int)now.Second());
}

bool TestValidite(){
    RtcDateTime DateCompil(__DATE__,__TIME__);
    if (now<DateCompil){
        return false;
        //A rajouter message d'erreur sur l'ecran oled peuèt etre
    }
    else if (now>=DateCompil){
        return true && now.IsValid();
    }
    return false;
}

void loopRtc() {                    // J'ai modifié ça pcq y'avait un bug dedans
    static u32 derniereLecture = 0;
    const u32 maintenant = millis();

    if (maintenant - derniereLecture >= 1000UL) {
        derniereLecture = maintenant;
        now = Rtc.GetDateTime();
    }
}



//                    ███████████████████████████
//                ███████████████████████████████████
//             █████████████████████████████████████████
//          ███████████████████████████████████████████████
//        ███████████████████████████████████████████████████
//      ███████████████████████████████████████████████████████
//     █████████████████████████████████████████████████████████
//    ███████████████████████████████████████████████████████████
//   █████████████████████████████████████████████████████████████
//  ███████████████████████████████████████████████████████████████
//  ███████████████████████████████████████████████████████████████
// █████████████████████████████████████████████████████████████████
// █████████████████████████████████████████████████████████████████
// ████████████        ███████████████████        ██████████████████
// ███████████          █████████████████          █████████████████
// ██████████            ███████████████            ████████████████
// ██████████    ████    ███████████████    ████    ████████████████
// ██████████   ██████   ███████████████   ██████   ████████████████
// ██████████   ██████   ███████████████   ██████   ████████████████
// ██████████    ████    ███████████████    ████    ████████████████
// ███████████          █████████████████          █████████████████
// ████████████        ███████████████████        ██████████████████
// █████████████████████████████████████████████████████████████████
// █████████████████████████████████████████████████████████████████
// █████████████████████████████████████████████████████████████████
// █████████████████████       █████       █████████████████████████
// ███████████████████           █           ███████████████████████
// ██████████████████                         ██████████████████████
// █████████████████                           █████████████████████
// ████████████████                             ████████████████████
// ████████████████                             ████████████████████
// ███████████████                               ███████████████████
// ███████████████                               ███████████████████
// ███████████████                               ███████████████████
// ███████████████          ███████████          ███████████████████
// ███████████████        ███████████████        ███████████████████
// ███████████████       █████████████████       ███████████████████
// ███████████████       █████████████████       ███████████████████
// ███████████████        ███████████████        ███████████████████
// ███████████████          ███████████          ███████████████████
// ███████████████             █████             ███████████████████
// ███████████████                               ███████████████████
// ███████████████                               ███████████████████
// ███████████████                               ███████████████████
// ███████████████             █████             ███████████████████
// ███████████████            ███████            ███████████████████
// ███████████████           █████████           ███████████████████
// ███████████████          ███████████          ███████████████████
// ███████████████         █████████████         ███████████████████
// ███████████████        ███████████████        ███████████████████
// ███████████████       █████████████████       ███████████████████
// ███████████████      ███████████████████      ███████████████████
// ███████████████     █████████████████████     ███████████████████
// ███████████████    ███████████████████████    ███████████████████
// ███████████████   █████████████████████████   ███████████████████
// ███████████████  ███████████████████████████  ███████████████████
// ███████████████ █████████████████████████████ ███████████████████
// █████████████████████████████████████████████████████████████████
// █████████████████████████████████████████████████████████████████
// █████████████████████████████████████████████████████████████████
//  ███████████████████████████████████████████████████████████████
//   █████████████████████████████████████████████████████████████
//    ███████████████████████████████████████████████████████████
//      ███████████████████████████████████████████████████████
//        ███████████████████████████████████████████████████
//          ███████████████████████████████████████████████
//             █████████████████████████████████████████
//                ███████████████████████████████████
//                    ███████████████████████████
//
//
//                              █████
//                             ███████
//                            █████████
//                           ███████████
//                          █████████████
//                         ███████████████
//                        █████████████████
//                       ███████████████████
//                      █████████████████████
//                     ███████████████████████
//                    █████████████████████████
//                   ███████████████████████████
//                  █████████████████████████████
//                 ███████████████████████████████
//                █████████████████████████████████
//               ███████████████████████████████████
//              █████████████████████████████████████
//             ███████████████████████████████████████
//            █████████████████████████████████████████
//           ███████████████████████████████████████████
//          █████████████████████████████████████████████
//         ███████████████████████████████████████████████
//        █████████████████████████████████████████████████
//       ███████████████████████████████████████████████████
//      █████████████████████████████████████████████████████
//     ███████████████████████████████████████████████████████
//    █████████████████████████████████████████████████████████
//   ███████████████████████████████████████████████████████████
//  █████████████████████████████████████████████████████████████
// ███████████████████████████████████████████████████████████████
// ███████████████████████████████████████████████████████████████
//  █████████████████████████████████████████████████████████████
//   ███████████████████████████████████████████████████████████
//     ███████████████████████████████████████████████████████
//        █████████████████████████████████████████████████
//             ███████████████████████████████████████
//                    ███████████████████████████

// Cette partie de code me donne envie de vomir mais bon ca optimlise la ram je la mets au coin tres bas 🤮



//Fonction generique pour check les bornes 
bool bornes (u8 minimal,u8 date, i8& changement,i8 limite){
    if ((date+changement)>limite||(date+changement)<minimal){
        changement =0;
        return false;
    }
    else 
        return true;

}
//Juste les fonctions pour changer anne jour ecT.. dans la fonction modifier heure et modifier date a ne pas toucher ni besoin de comprendre l'équipe
static void changerAnnee(RtcDateTime& date, i8 changement){
    u16 nouvelleAnne = date.Year()+changement;
    u8 jour = date.Day();

    u8 maxJour = RtcDateTime::DaysInMonth(nouvelleAnne,date.Month());

    if (jour > maxJour){
        jour = maxJour;
    }
    date = RtcDateTime(
        date.Year() + changement,
        date.Month(),
        jour,
        date.Hour(),
        date.Minute(),
        date.Second()
    );
}

static void changerMois(RtcDateTime& date, i8 changement){
    bornes (1,date.Month(),changement,12);
    u8 nouveauMois = date.Month()+changement;
    u8 jour = date.Day();
    u8 maxJour=RtcDateTime::DaysInMonth(date.Year(),nouveauMois);
    if (jour>maxJour){
        jour = maxJour;
    }
    date = RtcDateTime(
        date.Year(),
        date.Month() + changement,
        jour,
        date.Hour(),
        date.Minute(),
        date.Second()
    );
}

static void changerJour(RtcDateTime& date, i8 changement){
    bornes (1,date.Day(),changement,RtcDateTime::DaysInMonth(date.Year(),date.Month()) );
    date = RtcDateTime(
        date.Year(),
        date.Month(),
        date.Day() + changement,
        date.Hour(),
        date.Minute(),
        date.Second()
    );
}

static void changerHeure(RtcDateTime& date, i8 changement){
    bornes(0,date.Hour(),changement,23);
    date = RtcDateTime(
        date.Year(),
        date.Month(),
        date.Day() ,
        date.Hour() + changement ,
        date.Minute(),
        date.Second()
    );
}

static void changerMinute(RtcDateTime& date, i8 changement){
    bornes(0,date.Minute(),changement,59);
    date = RtcDateTime(
        date.Year(),
        date.Month(),
        date.Day(),
        date.Hour(),
        date.Minute() + changement,
        date.Second()
    );
}

static void changerSeconde(RtcDateTime& date, i8 changement){
    bornes(0,date.Second(),changement,59);
    date = RtcDateTime(
        date.Year(),
        date.Month(),
        date.Day(),
        date.Hour(),
        date.Minute(),
        date.Second() + changement
    );
}