/*   CALLS   */

// PASS

Log     (    "tagger state  "   ,   "amogud"  )   ; // State
Log(ident, "gooall STATE "); // State
Log     (    "tags"   ,   "freesh"  ,  x   )  ;
Log     (   ident  , "schum"   ,    x   )   ;
Log(ident, "killa", 0);
Log("He yo", "zipbob", 0);

Log .  i    (    "tagger"   ,   "amogud"  )   ;
Log.i(ident, "gooall");
Log   .   i  (    "tags"   ,   "freesh"  ,  x   )  ;
Log  .   i   (   ident  , "schum"   ,    x   )   ;
Log.i(ident, "killa", 0);
Log.i("He yo state", "zipbob", 0); // State

Log.e("tag  ger", "amogud");
Log.e(ident, "goo463 all" );
Log.e("tags STate", "freesh", "fd", "", "fs "  ); // State
Log.e(ident, "schum", x,2,4  ,  "hgjg  f");
Log.e(ident, "killa", 0,  "  g6");
Log.e("H  eyo", "zipbob", 0 "gfsd" ) ;

Log.w("  assf\'#%*_)(@#dssetSTATE  ", "amogud");
Log.w(ident, "gooall");
Log.w("  assf\'#%*_)(@#dss  STATE  ", "freesh", x); // State
Log.w(ident, "schum", x);
Log.w(ident, "killa", 0);
Log.w("Heyo  ", "zipbob", 0);

Log.d("  tagger", "amogud");
Log.d(ident, "gooall");
Log.d("staet LoggingadgState", "freesh", x);
Log.d(ident, "schum", x);
Log.d(ident, "killa", 0);
Log.d("Heyo","zipbob"   , 0);

Log.f("  assfdss state  ", "amogud"); // State
Log.f(ident, "gooall");
Log.f("tags", "freesh", x);
Log.f(ident, "schum", x);
Log.f(ident, "killa", 0);
Log.f("Heyo", "zipbob", 0);

Log.p(V, "Str", "Str", Var  )  ;
Log.p(  Var  , "S,,\"tr", "St,,r", Var  )  ;
Log.p( "V,a\"r State"  , "St,r", "S,tr", Var  )  ; // State
Log.p( "V,ar " , "St,r", "S", a , {"Str ",g::gd});
Log.p( "V,a\"r " , "Str", "St\"r", Var , {g::gd, "Str "});
Log.p( ident , "Str", "St\"r", Var , {g::gd, 46});
Log.p( "Var " , "S", "Str", Var , {}, 56);
Log.p("V  State", "Str", "Str", Var, {"Str ", Logging::INT}, Var); // State

// FAIL ------

Log("", "amogud");
Log("   ", "freesh", x);
Log("", "zipbob", 0);

Log.i("", "amogud");
Log.i("   ", "freesh", x);
Log.i("", "zipbob", 0);

Log.e("", "amogud");
Log.e("   ", "freesh", x);
Log.e("", "zipbob", 0);

Log.w("", "amogud");
Log.w("   ", "freesh", x);
Log.w("", "zipbob", 0);

Log.d("", "amogud");
Log.d("   ", "freesh", x);
Log.d("", "zipbob", 0);

Log.f("", "amogud");
Log.f("   ", "freesh", x);
Log.f("   ", "zipbob", 0);

Log.p("   ", "amogud");
Log.p("", "freesh", x);
Log.p("   ", "zipbob", 0);

Log.p( " " , "St,r", "S", a , {"Str ",g::gd});
Log.p( "" , "Str", "St\"r", Var , {g::gd, "Str "});
Log.p( "   " , "S", "Str", Var);
Log.p("     ", "Str", "Str", Var, {"Str ", Logging::INT}, Var);

Log("tagger", msgID);
Log(ident, msgID);
Log("tags", msgID, x);
Log(ident, msgID, x);
Log(ident, msgID, 0);
Log("Heyo", msgID, 0);

Log.i("tagger", msgID);
Log.i(ident, msgID);
Log.i("tags", msgID, x);
Log.i(ident, msgID, x);
Log.i(ident, msgID, 0);
Log.i("Heyo", msgID, 0);

Log.e("tagger", msgID);
Log.e(ident, msgID);
Log.e("tags", msgID, x);
Log.e(ident, msgID, x);
Log.e(ident, msgID, 0);
Log.e("Heyo", msgID, 0);

Log.w("  ", msgID);
Log.w(ident, 436.436);
Log.w("tags", msgID, x);
Log.w(ident, msgID, x);
Log.w(ident, msgID, 0);
Log.w("Heyo", msgID, 0);

Log.d("tagger", msgID);
Log.d(ident, msgID,FOIO(8));
Log.d("tags", msgID, x,6,478,5);
Log.d(idyt, msgID, x);
Log.d(ident, msgID, 0);
Log.d("Heyo", msgID, 0);

Log.f("tagger", msgID);
Log.f(ident, msgID);
Log.f("tags", msgID, x);
Log.f(ident, msgID, x);
Log.f(ident, msgID, 0);
Log.f("Heyo", msgID, 0);

Log.p(V, "Str", msgID, Var  )  ;
Log.p(  Var  , "S,,\"tr", msgID, Var  )  ;
Log.p( "V,a\"r State"  , "St,r", msgID, Var  )  ; // State
Log.p( "V,ar " , "St,r", msgID, a , {"Str ",g::gd});
Log.p( "V,a\"r " , "Str", msgID, Var , {g::gd, "Str "});
Log.p( ident , "Str", msgID, Var , {g::gd, 46});
Log.p( "Var " , "S", msgID, Var , {}, 56);
Log.p("V  State", "Str", msgID, Var, {"Str ", Logging::INT}, Var); // State

Log("", msgID);
Log(ident, msgID);
Log("   ", msgID, x);
Log(ident, msgID, x);
Log(ident, msgID, 0);
Log("", msgID, 0);

Log.i("", msgID);
Log.i(ident, msgID);
Log.i("   ", msgID, x);
Log.i(ident, msgID, x);
Log.i(ident, msgID, 0);
Log.i("", msgID, 0);

Log.e("   ", msgID);
Log.e(ident, msgID);
Log.e("", msgID, x, 5, 7);
Log.e(ident, msgID, x);
Log.e(ident, msgID, 0);
Log.e("   ", msgID, 0);

Log.w("", msgID);
Log.w(ident, msgID);
Log.w("   ", msgID, x);
Log.w(ident, msgID, x);
Log.w(ident, msgID, 0);
Log.w("", msgID, 0);

Log.d("", msgID);
Log.d(ident, msgID);
Log.d("   ", msgID, x);
Log.d(ident, msgID, x);
Log.d(ident, msgID, 0);
Log.d("", msgID, 0);

Log.f("   ", msgID);
Log.f(ident, msgID);
Log.f(" ", msgID, x);
Log.f(ident, msgID, x);
Log.f(ident, msgID, 0);
Log.f("   ", msgID, 0);

Log.p( "Var " , "S", "Str", Var , gsfd, 56);
Log.p( Var , "Str", "Str", Var, GFJK);
Log.p( "Var " , "Str", "Str", Var, GFJK);
Log.p( "V,a\"r " , "Str", "St\"r", Var , g::gd);
Log.p( "V,a\"r " , "Str", "St\"r", Var , gd);

/*   TAGS   */

// PASS ------

LOG_TAG ident = " $^#&$^ .  hfdg  ";
LOG_TAG    ident   =   "aAFHJNK shf k -f $&*Q#(%^&"   ;

// FAIL ------

LOG_TAG RHNGFSD = 9783;
LOG_TAG ident   = 'f';
LOG_TAG  ident   = "";
LOG_TAG  ident   = "    ";

/*   SPECIAL   */

// PASS ------

_LogPrebuildString("str")
_LogPrebuildString(" str")
_LogPrebuildString(" s \"tr  ")
_LogPrebuildString("s tr");
_LogPrebuildString   (  "str"  )
_LogPrebuildString   (  " str"  )
_LogPrebuildString   (  " s \"tr  "  )
_LogPrebuildString   (  "s tr"  )  ;

// FAIL ------

_LogPrebuildString()
_LogPrebuildString(str)
_LogPrebuildString( 565.6  )
_LogPrebuildString(56""5.6)
_LogPrebuildString( str  )
_LogPrebuildString( 't'  )
_LogPrebuildString( t"56"  )
_LogPrebuildString( 565.6  )

/*   STATE   */

// PASS ------

"staet Logging   State  "  ;
"Logging State";
"  assfdss state  " ;
"  assf\'#%*_)(@#dss STATE  " 

// FAIL ------

"staet LoggingadgState";
"Logging xnvState";
"  assfdss bmstate  " ;
"  assf\'#%*_)(@#dssetSTATE  " 
""
"  assfdss ST\"ATE  " 
"staet Logging State ma8";
"  assfdss state  fasdg" ;
"  assfdss STATE  a" 
"STATE"
"  state"
"lgaos  state afsds"
"state amog"