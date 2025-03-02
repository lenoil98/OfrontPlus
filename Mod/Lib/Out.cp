MODULE Out; (* DCW Brown. 2016-09-27 *)

IMPORT SYSTEM, Platform, Heap;

TYPE
  INTEGER = SYSTEM.INT16; LONGINT = SYSTEM.INT32; HUGEINT = SYSTEM.INT64;
  REAL = SYSTEM.REAL32; LONGREAL = SYSTEM.REAL64; CHAR = SYSTEM.CHAR8;

VAR
  IsConsole-: BOOLEAN;

  buf: ARRAY 128 OF CHAR;
  in: INTEGER;



PROCEDURE Flush*;
VAR error: Platform.ErrorCode;
BEGIN
  IF in > 0 THEN error := Platform.Write(Platform.StdOut, SYSTEM.ADR(buf), in) END;
  in := 0;
END Flush;

PROCEDURE Open*;
BEGIN
END Open;

PROCEDURE Char*(ch: CHAR);
BEGIN
  IF in >= LEN(buf) THEN Flush END;
  buf[in] := ch; INC(in);
  IF ch = 0AX THEN Flush END;
END Char;

PROCEDURE Length(IN s: ARRAY OF CHAR): LONGINT;
VAR l: LONGINT;
BEGIN l := 0; WHILE (l < LEN(s)) & (s[l] # 0X) DO INC(l) END; RETURN l
END Length;

PROCEDURE String*(IN str: ARRAY OF CHAR);
  VAR l: LONGINT;  error: Platform.ErrorCode;
BEGIN
  l := Length(str);
  IF in + l > LEN(buf) THEN Flush END;
  IF l > LEN(buf) THEN
    (* Doesn't fit buf. Bypass buffering. *)
    error := Platform.Write(Platform.StdOut, SYSTEM.ADR(str), l)
  ELSE
    SYSTEM.MOVE(SYSTEM.ADR(str), SYSTEM.ADR(buf[in]), l); INC(in, SHORT(l));
  END
END String;


PROCEDURE Int*(x, n: HUGEINT);
  CONST zero = ORD('0');
  VAR s: ARRAY 22 OF CHAR; i: INTEGER; negative: BOOLEAN;
BEGIN
  negative := x < 0;
  IF x = MIN(HUGEINT) THEN
    s := "8085774586302733229"; i := 19
  ELSE
    IF x < 0 THEN x := - x END;
    s[0] := SHORT(CHR(zero + (x MOD 10))); x := x DIV 10;
    i := 1; WHILE x # 0 DO
      s[i] := SHORT(CHR(zero + (x MOD 10)));
      x := x DIV 10;
      INC(i)
    END
  END;
  IF negative THEN s[i] := '-'; INC(i) END;
  WHILE n > i DO Char(' '); DEC(n) END;
  WHILE i > 0 DO DEC(i); Char(s[i]) END
END Int;


PROCEDURE Hex*(x, n: HUGEINT);
BEGIN
  IF n < 1 THEN n := 1 ELSIF n > 16 THEN n := 16 END;
  IF x >= 0 THEN
    WHILE (n < 16) & (SYSTEM.LSH(x, -4*n) # 0) DO INC(n) END
  END;
  x := SYSTEM.ROT(x, 4*(16-n));
  WHILE n > 0 DO
    x := SYSTEM.ROT(x,4); DEC(n);
    IF x MOD 16 < 10 THEN Char(SHORT(CHR((x MOD 16) + ORD('0'))))
    ELSE Char(SHORT(CHR((x MOD 16) - 10 + ORD('A')))) END
  END
END Hex;

PROCEDURE Ln*;
BEGIN String(Platform.NewLine); Flush;
END Ln;


(* Real and Longreal display *)

PROCEDURE digit(n: HUGEINT; VAR s: ARRAY OF CHAR; VAR i: INTEGER);
BEGIN
  DEC(i); s[i] := SHORT(CHR(n MOD 10 + 48));
END digit;

PROCEDURE prepend(IN t: ARRAY OF CHAR; VAR s: ARRAY OF CHAR; VAR i: INTEGER);
  VAR j: INTEGER; l: LONGINT;
BEGIN
  l := Length(t); IF l > i THEN l := i END;
  DEC(i, SHORT(l)); j := 0;
  WHILE j < l DO s[i+j] := t[j]; INC(j) END
END prepend;



PROCEDURE Ten*(e: INTEGER): LONGREAL;
VAR r, power: LONGREAL;
BEGIN r := 1.0E0; power := 1.0E1;
  WHILE e > 0 DO
    IF ODD(e) THEN r := r*power END;
    power := power*power; e := SHORT(e DIV 2)
  END;
  RETURN r
END Ten;

PROCEDURE -Entier64(x: LONGREAL): SYSTEM.INT64 "(LONGINT)(x)";

PROCEDURE RealP(x: LONGREAL; n: INTEGER; long: BOOLEAN);

(* RealP(x, n) writes the long real number x to the end of the output stream using an
   exponential form. If the textual representation of x requires m characters (including  a
   three-digit signed exponent), x is right adjusted in a field of Max(n, m) characters padded
   with blanks at the left end. A plus sign of the mantissa is not written.
   LONGREAL is 1/sign, 11/exponent, 52/significand *)

VAR
  e:   INTEGER;          (* Exponent field *)
  f:   HUGEINT;          (* Fraction field *)
  s:   ARRAY 30 OF CHAR; (* Buffer built backwards *)
  i:   INTEGER;          (* Index into s *)
  el:  INTEGER;          (* Exponent length *)
  x0:  LONGREAL;
  nn:  BOOLEAN;          (* Number negative *)
  en:  BOOLEAN;          (* Exponent negative *)
  m:   HUGEINT;          (* Mantissa digits *)
  d:   INTEGER;          (* Significant digit count to display *)
  dr:  INTEGER;          (* Number of insignificant digits that can be dropped *)

BEGIN
  e  := SYSTEM.VAL(INTEGER, (SYSTEM.VAL(HUGEINT, x) DIV 10000000000000L) MOD 800H);
  f  := SYSTEM.VAL(HUGEINT, x) MOD 10000000000000L;
  nn := (SYSTEM.VAL(HUGEINT, x) < 0) & ~((e = 7FFH) & (f # 0)); (* Ignore sign on Nan *)
  IF nn THEN DEC(n) END;

  i := LEN(s);
  IF e = 7FFH THEN (* NaN / Infinity *)
    IF f = 0 THEN prepend("Infinity", s, i) ELSE prepend("NaN", s, i) END
  ELSE
    (* Calculate number of significant digits caller has proposed space for, and
       number of digits to generate. *)
    IF long THEN
      el := 3;
      dr := SHORT(n-6);             (* Leave room for dp and '+D000' *)
      IF dr > 17 THEN dr := 17 END; (* Limit to max useful significant digits *)
      d := dr;                      (* Number of digits to generate *)
      IF d < 15 THEN d := 15 END    (* Generate enough digits to do trailing zero supporession *)
    ELSE
      el := 2;
      dr := SHORT(n-5);             (* Leave room for dp and '+E00' *)
      IF dr > 9 THEN dr := 9 END;   (* Limit to max useful significant digits *)
      d := dr;                      (* Number of digits to generate *)
      IF d < 6 THEN d := 6 END      (* Generate enough digits to do trailing zero supporession *)
    END;

    IF e = 0 THEN
      WHILE el > 0 DO DEC(i); s[i] := "0"; DEC(el) END;
      DEC(i); s[i] := "+";
      m := 0;
    ELSE
      IF nn THEN x := -x END;

      (* Scale e to be an exponent of 10 rather than 2 *)
      e := SHORT(SHORT(LONG(e - 1023) * 77 DIV 256));
      IF e >= 0 THEN x := x / Ten(e) ELSE x := Ten(SHORT(-e)) * x END ;
      IF x >= 10.0E0 THEN x := 0.1E0 * x; INC(e) END;

      (* Generate the exponent digits *)
      en := e < 0; IF en THEN e := SHORT(-e) END;
      WHILE el > 0 DO digit(e, s, i); e := SHORT(e DIV 10); DEC(el) END;
      DEC(i); IF en THEN s[i] := "-" ELSE s[i] := "+" END;

      (* Scale x to enough significant digits to reliably test for trailing
         zeroes or to the amount of space available, if greater. *)
      x0 := Ten(SHORT(d-1));
      x  := x0 * x;
      x  := x + 0.5E0; (* Do not combine with previous line as doing so
                          introduces a least significant bit difference
                          between 32 bit and 64 bit builds. *)
      IF x >= 10.0E0 * x0 THEN x := 0.1E0 * x; INC(e) END;
      m := Entier64(x)
    END;

    DEC(i); IF long THEN s[i] := "D" ELSE s[i] := "E" END;

    (* Drop trailing zeroes where caller proposes to use less space *)
    IF dr < 2 THEN dr := 2 END;
    WHILE (d > dr) & (m MOD 10 = 0) DO m := m DIV 10; DEC(d) END;

    (* Render significant digits *)
    WHILE d > 1 DO digit(m, s, i); m := m DIV 10; DEC(d) END;
    DEC(i); s[i] := '.';
    digit(m, s, i);
  END;

  (* Generate leading padding *)
  DEC(n, SHORT(LEN(s)-i)); WHILE n > 0 DO Char(" "); DEC(n) END;

  (* Render prepared number from right end of buffer s *)
  IF nn THEN Char("-") END;
  WHILE i < LEN(s) DO Char(s[i]); INC(i) END
END RealP;


PROCEDURE Real*(x: REAL; n: INTEGER);
BEGIN RealP(x, n, FALSE);
END Real;

PROCEDURE LongReal*(x: LONGREAL; n: INTEGER);
BEGIN RealP(x, n, TRUE);
END LongReal;

(* Convert LONGREAL: Write positive integer value of x into array d.
   The value is stored backwards, i.e. least significant digit
   first. n digits are written, with trailing zeros fill.
   On entry x has been scaled to the number of digits required. *)
PROCEDURE ConvertL*(x: LONGREAL; n: INTEGER; VAR d: ARRAY OF CHAR);
  VAR i, j, k: HUGEINT;
BEGIN
  IF x < 0 THEN x := -x END;
  k := 0;

  IF (SIZE(LONGINT) < 8) & (n > 9) THEN
    (* There are more decimal digits than can be held in a single LONGINT *)
    i := ENTIER(x /      1000000000.0E0);  (* The 10th and higher digits *)
    j := ENTIER(x - (i * 1000000000.0E0)); (* The low 9 digits *)
    (* First generate the low 9 digits. *)
    IF j < 0 THEN j := 0 END;
    WHILE k < 9 DO
      d[k] := SHORT(CHR(j MOD 10 + 48)); j := j DIV 10; INC(k)
    END;
    (* Fall through to generate the upper digits *)
  ELSE
    (* We can generate all the digits in one go. *)
    i := ENTIER(x);
  END;

  WHILE k < n DO
    d[k] := SHORT(CHR(i MOD 10 + 48)); i := i DIV 10; INC(k)
  END
END ConvertL;

PROCEDURE Expo*(x: REAL): INTEGER;
  VAR i: INTEGER;
BEGIN
  SYSTEM.GET(SYSTEM.ADR(x)+2, i);
  RETURN SHORT((i DIV 128) MOD 256)
END Expo;

PROCEDURE RealFix* (x: REAL; n, k: INTEGER);
  CONST maxD = 9;
  VAR e, i: INTEGER; sign: CHAR; x0: REAL;
    d: ARRAY maxD OF CHAR;

  PROCEDURE seq(ch: CHAR; n: INTEGER);
  BEGIN WHILE n > 0 DO Char(ch); DEC(n) END
  END seq;

  PROCEDURE dig(n: INTEGER);
  BEGIN
    WHILE n > 0 DO
      DEC(i); Char(d[i]); DEC(n)
    END
  END dig;

BEGIN e := Expo(x);
  IF k < 0 THEN k := 0 END;
  IF e = 0 THEN seq(" ", SHORT(n-k-2)); Char("0"); seq(" ", SHORT(k+1))
  ELSIF e = 255 THEN String(" NaN"); seq(" ", SHORT(n-4))
  ELSE e := SHORT((e - 127) * 77 DIV 256);
    IF x < 0 THEN sign := "-"; x := -x ELSE sign := " " END;
    IF e >= 0 THEN  (*x >= 1.0,  77/256 = log 2*) x := SHORT(x/Ten(e))
      ELSE (*x < 1.0*) x := SHORT(Ten(SHORT(-e)) * x)
    END;
    IF x >= 10.0 THEN x := 0.1*x; INC(e) END;
    (* 1 <= x < 10 *)
    IF k+e >= maxD-1 THEN k := SHORT(maxD-1-e)
      ELSIF k+e < 0 THEN k := SHORT(-e); x := 0.0
    END;
    x0 := SHORT(Ten(SHORT(k+e))); x := x0*x + 0.5;
    IF x >= 10.0*x0 THEN INC(e) END;
    (*e = no. of digits before decimal point*)
    INC(e); i := SHORT(k+e); ConvertL(x, i, d);
    IF e > 0 THEN
      seq(" ", SHORT(n-e-k-2)); Char(sign); dig(e);
      Char("."); dig(k)
    ELSE seq(" ", SHORT(n-k-3));
      Char(sign); Char("0"); Char(".");
      seq("0", SHORT(-e)); dig(SHORT(k+e))
    END
  END
END RealFix;

BEGIN
  IsConsole := Platform.IsConsole(Platform.StdOut);
  in := 0
END Out.
