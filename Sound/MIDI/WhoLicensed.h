// Who.h

#define EVAL_COPY				0
#define EVAL_END_MONTH		2
#define EVAL_END_DAY			1
#define EVAL_END_YEAR			1996

#define INTERNAL				6

// Set the define WHO to the licensee and rebuild. This will encode the library with the license agreement,
// and the company contact
//
#define WHO 					INTERNAL
//
//
// 0 opcode
// 1 presage
// 2 broderbund
// 3 maxis
// 4 learning company
// 5 Electronic Arts			; was Clairis
// 6 internal use
// 7 Dynamix/JTP			; was Apple Computer, Inc. Bruce Leak
// 8 Spectrum Holobyte
// 9 Velocity Development Corp.
// 10 Interplay
// 11 Microsoft			; was Microprose
// 12 Magellan				; was Nova Corporation
// 13 MECC Software
// 14 Knowledge Adventure
// 15 Dreamer's Guild
// 16 Software Toolworks/Mindscape, Inc
// 17 Berkeley Systems, Inc
// 18 Trilobyte
// 19 Greendragon			; was Bungie
// 20 Activision
// 21 Alliance Interactive Sotware, Inc
// 22 Domark, Inc
// 23 MagicQuest
// 24 Soleil Software
// 25 Virgin Interactive
// 26 Ere Informatique
// 27 Norman Franke (SoundApp)
// 28 Daniel Reznick R/GA Studios eval only
// 29 CinemaTronics; Kevin Gliner
// 30 Stick Man Games; John Morris eval only
// 31 Viacom New Media; Cliff Falls
// 32 Blizzard Entertainment; Bill Roper eval only
// 33 Iona Software Limited; Paul Milne
// 34 Lion Entertainment, Inc; Bruce Burkhalter
// 35 The Software Works, Inc; Pierre Maloka
// 36 Accolade, Inc; James Vitales
// 37 Jump Software, Inc; Mukunda  Penugonde
// 38 QKumber, Inc; Borris Razhas
// 39 Norman Franke III
// 40 Mark Thomas
// 41 Netscape, Inc
// 42 Westwood
// 43 Artemis Research, Inc

#undef LICENSEE
#if WHO == 0
#define LICENSEE		"Licensed to Opcode Systems, Inc: Koord Taylor"
#endif

#if WHO == 1
#define LICENSEE		"Licensed to Presage Software Developement, Inc"
#endif

#if WHO == 2
#define LICENSEE		"Licensed to Br¿derbund Software, Inc: Tom Rettig"
#endif

#if WHO == 3
#define LICENSEE		"Licensed to Maxis, Inc"
#endif

#if WHO == 4
#define LICENSEE		"Licensed to The Learning Company"
#endif

#if WHO == 5
#define LICENSEE		"Licensed to Electronic Arts, Inc"
#endif

#if WHO == 6
#define LICENSEE		"(c) Copyright 1989-1996 Steve Hales"
#endif

#if WHO == 7
#define LICENSEE		"Licensed to Dynamix/JTP"
#endif

#if WHO == 8
#define LICENSEE		"Licensed to Sphere, Inc"
#endif

#if WHO == 9
#define LICENSEE		"Licensed to Velocity Development Corporation: Moses Ma"
#endif

#if WHO == 10
#define LICENSEE		"Licensed to Interplay Productions:  Allen Pavlish"
#endif

#if WHO == 11
#define LICENSEE		"Evaluation to Mircoprose Software, Inc:  David Brewer"
#endif

#if WHO == 12
#define LICENSEE		"Licensed to Magellan Systems"
#endif

#if WHO == 13
#define LICENSEE		"Licensed to MECC Software:  John Krenz"
#endif

#if WHO == 14
#define LICENSEE		"Licensed for Knowledge Adventure:  David Gobel"
#endif

#if WHO == 15
#define LICENSEE		"Licensed to Dreamer's Guild"
#endif

#if WHO == 16
#define LICENSEE		"Licensed to Software Toolworks, Inc/Mindscape, Inc"
#endif

#if WHO == 17
#define LICENSEE		"Licensed to Berkeley Systems, Inc"
#endif

#if WHO == 18
#define LICENSEE		"Licensed to Trilobyte"
#endif

#if WHO == 19
#define LICENSEE		"Licensed to Greendragon; Howard Shere"
#endif

#if WHO == 20
#define LICENSEE		"Licensed to Activision, Inc"
#endif

#if WHO == 21
#define LICENSEE		"Licensed to Alliance Interactive Sotware, Inc"
#endif

#if WHO == 22
#define LICENSEE		"Licensed to Domark, Inc"
#endif

#if WHO == 23
#define LICENSEE		"Evaluation to MagicQuest"
#endif

#if WHO == 24
#define LICENSEE		"Licensed to Soleil Software"
#endif

#if WHO == 25
#define LICENSEE		"Licensed to Virgin Sound and Vision"
#endif

#if WHO == 26
#define LICENSEE		"Licensed to Ere Informatique"
#endif

#if WHO == 27
#define LICENSEE		"Licensed to Norman Franke (SoundApp)"
#endif

#if WHO == 28
#define LICENSEE		"Licensed to R/GA Digital Studios: Daniel Reznick"
#endif

#if WHO == 29
#define LICENSEE		"Licensed to CinemaTronics; Kevin Gliner"
#endif

#if WHO == 30
#define LICENSEE		"Licensed to Stick Man Games; John Morris"
#endif

#if WHO == 31
#define LICENSEE		"Licensed to Viacom New Media; Cliff Falls"
#endif

#if WHO == 32
#define LICENSEE		"Eval to Blizzard Entertainment; Bill Roper"
#endif

#if WHO == 33
#define LICENSEE		"Licensed to Iona Software Limited; Paul Milne"
#endif

#if WHO == 34
#define LICENSEE		"Licensed to Lion Entertainment, Inc; Bruce Burkhalter"
#endif

#if WHO == 35
#define LICENSEE		"Licensed to The Software Works, Inc; Pierre Maloka"
#endif

#if WHO == 36
#define LICENSEE		"Licensed to Accolade, Inc; James Vitales"
#endif

#if WHO == 37
#define LICENSEE		"Licensed to Jump Software, Inc; eval to Mukunda  Penugonde"
#endif

#if WHO == 38
#define LICENSEE		"Licensed to QKumber, Inc; Borris Razhas"
#endif

#if WHO == 39
#define LICENSEE		"Licensed to Norman Franke III"
#endif

#if WHO == 40
#define LICENSEE		"Evaluation to Mark Thomas"
#endif

#if WHO == 41
#define LICENSEE		"Evaluation to Netscape Communications, Inc"
#endif

#if WHO == 42
#define LICENSEE		"Evaluation to Westwood, Inc"
#endif

#if WHO == 43
#define LICENSEE		"Evaluation to Artemis Research, Inc"
#endif

extern void * CodeSignature(void);

#ifndef LICENSEE
#ERROR LICENSEE NOT DEFINED
#endif
