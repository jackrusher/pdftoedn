//
// Copyright 2016-2019 Ed Porras
//
// This file is part of pdftoedn.
//
// pdftoedn is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// pdftoedn is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with pdftoedn.  If not, see <http://www.gnu.org/licenses/>.

namespace pdftoedn {

    const char* DEFAULT_FONT_MAP =
        "{ " \
        "    \"fontMaps\": { " \
        "        \"standardFonts\": " \
        "        { \"entries\": " \
        "          [ " \
        "              [ \"_comment_\" ], " \
        "              [ \"Helvetica-Condensed-Bold\",                      \"Arial Narrow\",        [ \"bold\" ],         [ \"standardMap\" ] ], " \
        "              [ \"Helvetica-Condensed\",                           \"Arial Narrow\",        [],                   [ \"standardMap\" ] ], " \
        "              [ \"HelveticaNeue\",                                 \"Arial Narrow\",        [],                   [ \"standardMap\" ] ], " \
        "              [ \"Helvetica\",                                     \"Helvetica\",           [],                   [ \"standardMap\" ] ], " \
        "              [ \"ArialNarrow\",                                   \"Arial Narrow\",        [],                   [ \"standardMap\" ] ], " \
        "              [ \"Arial-Black\",                                   \"Arial Black\",         [],                   [ \"standardMap\" ] ], " \
        "              [ \"Arial\",                                         \"Arial\",               [],                   [ \"standardMap\" ] ], " \
        "              [ \"Courier\",                                       \"Courier\",             [],                   [ \"standardMap\" ] ], " \
        "              [ \"Garamond\",                                      \"Garamond\",            [],                   [ \"standardMap\" ] ], " \
        "              [ \"LucidaConsole\",                                 \"Courier\",             [],                   [ \"standardMap\" ] ], " \
        "              [ \"Myriad\",                                        \"Arial\",               [],                   [ \"standardMap\" ] ], " \
        "              [ \"Impact\",                                        \"Impact\",              [],                   [ \"standardMap\" ] ], " \
        "              [ \"NewCenturySchlbk\",                              \"Times\",               [],                   [ \"standardMap\" ] ], " \
        "              [ \"InterleafSymbols\",                              \"Symbol\" ], " \
        "              [ \"Symbol\",                                        \"Symbol\",              [],                   [ \"symclasMap\", \"standardMap\" ] ], " \
        "              [ \"Times\",                                         \"Times\",               [],                   [ \"standardMap\" ] ], " \
        "              [ \"Comic\",                                         \"Comic Sans MS\",       [],                   [ \"standardMap\" ] ], " \
        "              [ \"ZapfDingbats\",                                  \"Zapf Dingbats\",       [],                   [ \"zapfdingbatsMap\", \"standardMap\" ] ], " \
        "              [ \"WingDings\",                                     \"Wingdings\" ] " \
        "          ] " \
        "        }, " \
        "        \"websafeFallbackFonts\": " \
        "        { \"entries\": " \
        "          [ " \
        "              [ \"_comment_\", [ \"Myriad == Myriad Pro, Trebuchet MS, Arial, Sans-Serif\", " \
        "                                 \"Garamond, Times New Roman, Serif\" , " \
        "                                 \"Palatino, Palatino Linotype, Serif\", " \
        "                                 \"Impact, Haettenschweiler, Sans-Serif\" , " \
        "                                 \"Tahoma, Geneva, Sans-Serif\", " \
        "                                 \"Lucida Sans Unicode, Lucida Grande, Sans-Serif\", " \
        "                                 \"Futura, Verdana, Sans-Serif\" ] ], " \
        "              [ \"Avenir\",                                      \"Arial\" ], " \
        "              [ \"LucidaGrande\",                                \"Arial\",                 [],                   [ \"standardMap\" ] ], " \
        "              [ \"Palatino-Roman\",                              \"Times New Roman\",       [],                   [ \"standardMap\" ] ], " \
        "              [ \"Palatino\",                                    \"Palatino\",              [],                   [ \"standardMap\" ] ], " \
        "              [ \"Calibri\",                                     \"Arial Narrow\",          [],                   [ \"standardMap\" ] ], " \
        "              [ \"Bookman-Light\",                               \"Times\",                 [],                   [ \"standardMap\" ] ], " \
        "              [ \"BookmanOldStyle\",                             \"Times\",                 [],                   [ \"standardMap\" ] ], " \
        "              [ \"AmericanTypewriter\",                          \"Courier\",               [],                   [ \"standardMap\" ] ], " \
        "              [ \"AvantGarde\",                                  \"Century Gothic\",        [],                   [ \"standardMap\" ] ], " \
        "              [ \"Frutiger\",                                    \"Arial Narrow\",          [],                   [ \"standardMap\" ] ], " \
        "              [ \"MicrosoftSansSerif\",                          \"Arial Narrow\",          [],                   [ \"standardMap\" ] ], " \
        "              [ \"Futura\",                                      \"Century Gothic\",        [],                   [ \"standardMap\" ] ], " \
        "              [ \"TrebuchetMS\",                                 \"Trebuchet MS\",          [],                   [ \"standardMap\" ] ], " \
        "              [ \"century\",                                     \"Times\",                 [],                   [ \"standardMap\" ] ], " \
        "              [ \"LetterGothic\",                                \"Andale Mono\",           [],                   [ \"standardMap\" ] ], " \
        "              [ \"BookAntiqua\",                                 \"Times\",                 [],                   [ \"standardMap\" ] ], " \
        "              [ \"Cambria\",                                     \"Arial\",                 [],                   [ \"standardMap\" ] ], " \
        "              [ \"Tahoma\",                                      \"Tahoma\",                [],                   [ \"standardMap\" ] ], " \
        "              [ \"Universal-NewswithCommPi\",                    \"Symbol\",                [],                   [ \"universalNewsMap\" ] ], " \
        "              [ \"Univers-Condensed\",                           \"Arial Narrow\",          [],                   [ \"standardMap\" ] ], " \
        "              [ \"Univers\",                                     \"Arial\",                 [],                   [ \"standardMap\" ] ], " \
        "              [ \"msgothic\",                                    \"MS Gothic\",             [],                   [ \"standardMap\" ] ], " \
        "              [ \"ms-gothic\",                                   \"MS Gothic\",             [],                   [ \"standardMap\" ] ], " \
        "              [ \"mspgothic\",                                   \"MS PGothic\",            [],                   [ \"standardMap\" ] ], " \
        "              [ \"ms-pgothic\",                                  \"MS PGothic\",            [],                   [ \"standardMap\" ] ], " \
        "              [ \"msmincho\",                                    \"MS Mincho\",             [],                   [ \"standardMap\" ] ], " \
        "              [ \"ms-mincho\",                                   \"MS Mincho\",             [],                   [ \"standardMap\" ] ], " \
        "              [ \"mspmincho\",                                   \"MS PMincho\",            [],                   [ \"standardMap\" ] ], " \
        "              [ \"ms-pmincho\",                                  \"MS PMincho\",            [],                   [ \"standardMap\" ] ], " \
        "              [ \"AndaleMono\",                                  \"Andale Mono\" ], " \
        "              [ \"europeanpi\",                                  \"Symbol\",                [],                   [ \"europeanPiMap\", \"standardMap\" ] ] " \
        "          ] " \
        "        }, " \
        "        \"miscEmbeddedFonts\": " \
        "        { \"entries\": " \
        "          [ " \
        "              [ \"_comment_\", \"some embedded fonts we've already mapped\" ], " \
        "              [ \"swiss721Narrowswa\",                           \"Helvetica\" ], " \
        "              [ \"Swiss721SWA\",                                 \"Helvetica\",             [],                       [ \"swissMap\" ] ], " \
        "              [ \"Swis721bt\",                                   \"Helvetica\" ], " \
        "              [ \"mthmtha\",                                     \"Symbol\",                [],                       [ \"mthmthaMap\" ] ], " \
        "              [ \"wstcomisanmbi\",                               \"Comic Sans MS\",         [ \"bold\", \"italic\" ] ], " \
        "              [ \"wstcomisanmb\",                                \"Comic Sans MS\",         [ \"bold\" ] ], " \
        "              [ \"wstcomisanmi\",                                \"Comic Sans MS\",         [ \"italic\" ] ], " \
        "              [ \"wstcomisanm\",                                 \"Comic Sans MS\" ], " \
        "              [ \"wstdutchbi\",                                  \"Times\",                 [ \"bold\", \"italic\" ], [ \"swissMap\", \"standardMap\" ] ], " \
        "              [ \"wstdutchb\",                                   \"Times\",                 [ \"bold\" ],             [ \"swissMap\", \"standardMap\" ] ], " \
        "              [ \"wstdutchi\",                                   \"Times\",                 [ \"italic\" ],           [ \"swissMap\", \"standardMap\" ] ], " \
        "              [ \"wstdutch\",                                    \"Times\",                 [],                       [ \"swissMap\", \"standardMap\" ] ], " \
        "              [ \"wstgarapsbi\",                                 \"Garamond\",              [ \"bold\", \"italic\" ], [ \"standardMap\" ] ], " \
        "              [ \"wstgarapsb\",                                  \"Garamond\",              [ \"bold\" ],             [ \"standardMap\" ] ], " \
        "              [ \"wstgarapsi\",                                  \"Garamond\",              [ \"italic\" ],           [ \"standardMap\" ] ], " \
        "              [ \"wstgaraps\",                                   \"Garamond\",              [],                       [ \"standardMap\" ] ], " \
        "              [ \"wstpalapsbi\",                                 \"Palatino\",              [ \"bold\", \"italic\" ], [ \"standardMap\" ] ], " \
        "              [ \"wstpalapsb\",                                  \"Palatino\",              [ \"bold\" ],             [ \"standardMap\" ] ], " \
        "              [ \"wstpalapsi\",                                  \"Palatino\",              [ \"italic\" ],           [ \"standardMap\" ] ], " \
        "              [ \"wstpalaps\",                                   \"Palatino\",              [],                       [ \"standardMap\" ] ], " \
        "              [ \"wstsouvpsbi\",                                 \"Souvenir\",              [ \"bold\", \"italic\" ], [ \"standardMap\" ] ], " \
        "              [ \"wstsouvpsb\",                                  \"Souvenir\",              [ \"bold\" ],             [ \"standardMap\" ] ], " \
        "              [ \"wstsouvpsi\",                                  \"Souvenir\",              [ \"italic\" ],           [ \"standardMap\" ] ], " \
        "              [ \"wstsouvps\",                                   \"Souvenir\",              [],                       [ \"standardMap\" ] ], " \
        "              [ \"wstswissbi\",                                  \"Helvetica\",             [ \"bold\", \"italic\" ], [ \"swissMap\", \"standardMap\" ] ], " \
        "              [ \"wstswissb\",                                   \"Helvetica\",             [ \"bold\" ],             [ \"swissMap\", \"standardMap\" ] ], " \
        "              [ \"wstswissi\",                                   \"Helvetica\",             [ \"italic\" ],           [ \"swissMap\", \"standardMap\" ] ], " \
        "              [ \"wstswiss\",                                    \"Helvetica\",             [],                       [ \"swissMap\", \"standardMap\" ] ], " \
        "              [ \"wstcentbs\",                                   \"Helvetica\",             [ \"bold\" ],             [ \"standardMap\" ] ]," \
        "              [ \"wsthelvblpsbi\",                               \"Helvetica\",             [ \"bold\", \"italic\" ], [ \"swissMap\", \"standardMap\" ] ]," \
        "              [ \"wsthelvblpsb\",                                \"Helvetica\",             [ \"bold\" ],             [ \"swissMap\", \"standardMap\" ] ]," \
        "              [ \"wsthelvltps\",                                 \"Helvetica\",             [],                       [ \"swissMap\", \"standardMap\" ] ]," \
        "              [ \"wsthelvnpsb\",                                 \"Arial Narrow\",          [ \"bold\" ],             [ \"swissMap\", \"standardMap\" ] ]," \
        "              [ \"wsthelvnps\",                                  \"Arial Narrow\",          [],                       [ \"swissMap\", \"standardMap\" ] ]," \
        "              [ \"symclas\",                                     \"Symbol\",                [],                       [ \"symclasMap\", \"standardMap\" ] ]" \
        "          ]" \
        "        }" \
        "    }," \
        "    \"glyphMaps\":{" \
        "        \"standardMap\":" \
        "        {" \
        "            \".notdef\":         160," \
        "            \"C0127\":          8221," \
        "            \"C0177\":           177," \
        "            \"C0186\":           176," \
        "            \"C0258\":          8239," \
        "            \"C0259\":           160," \
        "            \"C0260\":          8194," \
        "            \"C0261\":          8195," \
        "            \"C0262\":          8211," \
        "            \"C0266\":          8212," \
        "            \"alpha\":           945," \
        "            \"ampersand\":        38," \
        "            \"arrowboth\":      8596," \
        "            \"arrowdblleft\":   8656," \
        "            \"arrowdblright\":  8658," \
        "            \"arrowdblboth\":   8660," \
        "            \"arrowdown\":      8595," \
        "            \"arrowleft\":      8592," \
        "            \"arrowright\":     8594," \
        "            \"arrowup\":        8593," \
        "            \"arrowhorizex\":   9135," \
        "            \"asciitilde\":      126," \
        "            \"tilde\":           126," \
        "            \"asterisk\":         42," \
        "            \"at\":               64," \
        "            \"bullet\":         8226," \
        "            \"dagger\":         8224," \
        "            \"degree\":          176," \
        "            \"diamond\":        9830," \
        "            \"divide\":          247," \
        "            \"dotmath\":         183," \
        "            \"ellipsis\":       8230," \
        "            \"endash\":         8211," \
        "            \"emdash\":         8212," \
        "            \"ff\":            64256," \
        "            \"f_f\":           64256," \
        "            \"fi\":            64257," \
        "            \"f_i\":           64257," \
        "            \"fl\":            64258," \
        "            \"f_l\":           64258," \
        "            \"ffi\":           64259," \
        "            \"f_f_i\":         64259," \
        "            \"ffl\":           64260," \
        "            \"f_f_l\":         64260," \
        "            \"minus\":          8722," \
        "            \"mu\":              956," \
        "            \"multiply\":        215," \
        "            \"notequal\":       8800," \
        "            \"Omega\":          8486," \
        "            \"plusminus\":       177," \
        "            \"periodcentered\":  183," \
        "            \"quoteleft\":      8216," \
        "            \"quoteright\":     8217," \
        "            \"quotedblleft\":   8220," \
        "            \"quotedblright\":  8221," \
        "            \"thin\":           8201," \
        "            \"trademark\":      8482," \
        "            \"trademarkserif\": 8482," \
        "            \"space\":            32," \
        "            \"exclam\":           33," \
        "            \"quotedbl\":         34," \
        "            \"numbersign\":       35," \
        "            \"dollar\":           36," \
        "            \"percent\":          37," \
        "            \"quotesingle\":      39," \
        "            \"parenleft\":        40," \
        "            \"parenright\":       41," \
        "            \"plus\":             43," \
        "            \"comma\":            44," \
        "            \"hyphen\":           45," \
        "            \"period\":           46," \
        "            \"slash\":            47," \
        "            \"zero\":             48," \
        "            \"one\":              49," \
        "            \"two\":              50," \
        "            \"three\":            51," \
        "            \"four\":             52," \
        "            \"five\":             53," \
        "            \"six\":              54," \
        "            \"seven\":            55," \
        "            \"eight\":            56," \
        "            \"nine\":             57," \
        "            \"colon\":            58," \
        "            \"semicolon\":        59," \
        "            \"lambda\":          955," \
        "            \"less\":             60," \
        "            \"equal\":            61," \
        "            \"lessequal\":      8804," \
        "            \"greater\":          62," \
        "            \"greaterequal\":   8805," \
        "            \"question\":         63," \
        "            \"A\":                65," \
        "            \"B\":                66," \
        "            \"C\":                67," \
        "            \"D\":                68," \
        "            \"E\":                69," \
        "            \"F\":                70," \
        "            \"G\":                71," \
        "            \"H\":                72," \
        "            \"I\":                73," \
        "            \"J\":                74," \
        "            \"K\":                75," \
        "            \"L\":                76," \
        "            \"M\":                77," \
        "            \"N\":                78," \
        "            \"O\":                79," \
        "            \"P\":                80," \
        "            \"Q\":                81," \
        "            \"R\":                82," \
        "            \"S\":                83," \
        "            \"T\":                84," \
        "            \"U\":                85," \
        "            \"V\":                86," \
        "            \"W\":                87," \
        "            \"X\":                88," \
        "            \"Y\":                89," \
        "            \"Z\":                90," \
        "            \"bracketleft\":      91," \
        "            \"backslash\":        92," \
        "            \"bracketright\":     93," \
        "            \"asciicircum\":      94," \
        "            \"underscore\":       95," \
        "            \"grave\":            96," \
        "            \"a\":                97," \
        "            \"b\":                98," \
        "            \"c\":                99," \
        "            \"d\":               100," \
        "            \"e\":               101," \
        "            \"f\":               102," \
        "            \"g\":               103," \
        "            \"h\":               104," \
        "            \"i\":               105," \
        "            \"j\":               106," \
        "            \"k\":               107," \
        "            \"l\":               108," \
        "            \"m\":               109," \
        "            \"n\":               110," \
        "            \"o\":               111," \
        "            \"p\":               112," \
        "            \"q\":               113," \
        "            \"r\":               114," \
        "            \"s\":               115," \
        "            \"t\":               116," \
        "            \"u\":               117," \
        "            \"v\":               118," \
        "            \"w\":               119," \
        "            \"x\":               120," \
        "            \"y\":               121," \
        "            \"z\":               122," \
        "            \"braceleft\":       123," \
        "            \"bar\":             124," \
        "            \"braceright\":      125," \
        "            \"exclamdown\":      161," \
        "            \"cent\":            162," \
        "            \"section\":         167," \
        "            \"copyright\":       169," \
        "            \"copyrightsans\":   63721," \
        "            \"copyrightserif\":  169," \
        "            \"guillemotleft\":   171," \
        "            \"registered\":      174," \
        "            \"registersans\":    174," \
        "            \"registered.alt\":  174," \
        "            \"registerserif\":   63194," \
        "            \"acute\":           180," \
        "            \"cedilla\":         184," \
        "            \"onesuperior\":     185," \
        "            \"ordmasculine\":    186," \
        "            \"guillemotright\":  187," \
        "            \"onequarter\":      188," \
        "            \"onehalf\":         189," \
        "            \"threequarters\":   190," \
        "            \"questiondown\":    191," \
        "            \"Atilde\":          195," \
        "            \"Adieresis\":       196," \
        "            \"Aring\":           197," \
        "            \"Egrave\":          200," \
        "            \"Eacute\":          201," \
        "            \"Eth\":             208," \
        "            \"Ntilde\":          209," \
        "            \"Oacute\":          211," \
        "            \"Odieresis\":       214," \
        "            \"Oslash\":          216," \
        "            \"Uacute\":          218," \
        "            \"Udieresis\":       220," \
        "            \"germandbls\":      223," \
        "            \"agrave\":          224," \
        "            \"aacute\":          225," \
        "            \"acircumflex\":     226," \
        "            \"atilde\":          227," \
        "            \"adieresis\":       228," \
        "            \"aring\":           229," \
        "            \"ccedilla\":        231," \
        "            \"egrave\":          232," \
        "            \"eacute\":          233," \
        "            \"ecircumflex\":     234," \
        "            \"edieresis\":       235," \
        "            \"igrave\":          236," \
        "            \"iacute\":          237," \
        "            \"icircumflex\":     238," \
        "            \"idieresis\":       239," \
        "            \"ntilde\":          241," \
        "            \"ograve\":          242," \
        "            \"oacute\":          243," \
        "            \"ocircumflex\":     244," \
        "            \"otilde\":          245," \
        "            \"odieresis\":       246," \
        "            \"oslash\":          248," \
        "            \"ugrave\":          249," \
        "            \"uacute\":          250," \
        "            \"ucircumflex\":     251," \
        "            \"udieresis\":       252," \
        "            \"yacute\":          253," \
        "            \"cacute\":          263," \
        "            \"Ccaron\":          268," \
        "            \"ccaron\":          269," \
        "            \"dcaron\":          271," \
        "            \"ecaron\":          283," \
        "            \"lacute\":          314," \
        "            \"nacute\":          324," \
        "            \"ncaron\":          328," \
        "            \"Rcaron\":          344," \
        "            \"rcaron\":          345," \
        "            \"Scaron\":          352," \
        "            \"scaron\":          353," \
        "            \"tcaron\":          357," \
        "            \"uring\":           367," \
        "            \"zdotaccent\":      380," \
        "            \"zcaron\":          382," \
        "            \"phi\":             966," \
        "            \"caron\":           711," \
        "            \"ring\":            730," \
        "            \"quotedblbase\":   8222," \
        "            \"minute\":         8242," \
        "            \"emptyset\":       8709," \
        "            \"filledbox\":      9632" \
        "        }," \
        "        \"symclasMap\":" \
        "        {" \
        "            \"C0033\":           185," \
        "            \"C0034\":          9654," \
        "            \"C0035\":           179," \
        "            \"C0036\":          8308," \
        "            \"C0045\":         10063," \
        "            \"C0058\":         10038," \
        "            \"C0064\":           178," \
        "            \"C0065\":          9664," \
        "            \"C0066\":          9660," \
        "            \"C0068\":          8226," \
        "            \"C0069\":          9426," \
        "            \"C0070\":          9679," \
        "            \"C0072\":          9632," \
        "            \"C0073\":          8243," \
        "            \"C0074\":          9632," \
        "            \"C0075\":          9733," \
        "            \"C0076\":          9733," \
        "            \"C0082\":           174," \
        "            \"C0083\":          8226," \
        "            \"C0086\":          9633," \
        "            \"C0089\":          9650," \
        "            \"C0095\":           176," \
        "            \"C0102\":          9675," \
        "            \"C0105\":          8242," \
        "            \"C0106\":          9633," \
        "            \"C0114\":           174," \
        "            \"C0116\":          8482," \
        "            \"C0122\":          9670" \
        "        }," \
        "        \"europeanPiMap\":" \
        "        {" \
        "            \"L50107\":         9711," \
        "            \"L50128\":          174," \
        "            \"L50132\":         9633," \
        "            \"L50188\":         9679," \
        "            \"_comment_\": { \"L50107\":        9675 }," \
        "            \"L50260\":         9426," \
        "            \"L50539\":         9679," \
        "            \"L50772\":         9660," \
        "            \"L50773\":         9651," \
        "            \"L50774\":         9650," \
        "            \"L50775\":         9664," \
        "            \"L50776\":         9654," \
        "            \"L50887\":         9670," \
        "            \"L52932\":         9655" \
        "        }," \
        "        \"eqmbsmthMap\":" \
        "        {" \
        "            \"space\":            32," \
        "            \"C0032\":           215," \
        "            \"C0034\":           177," \
        "            \"C0036\":           177," \
        "            \"C0041\":            43," \
        "            \"C0064\":           183," \
        "            \"C0082\":          8734," \
        "            \"C0088\":           126," \
        "            \"C0116\":            60," \
        "            \"C0117\":            62," \
        "            \"C0120\":          8804," \
        "            \"C0121\":          8805," \
        "            \"C0224\":          8660," \
        "            \"C0281\":          8853," \
        "            \"C0284\":          8854," \
        "            \"C0296\":          8853," \
        "            \"C0298\":          8854," \
        "            \"C0527\":          9633," \
        "            \"C0545\":          9127," \
        "            \"C0546\":          9129," \
        "            \"C0551\":          9130" \
        "        }," \
        "        \"mthmthaMap\":" \
        "        {" \
        "            \"C20\":            8804," \
        "            \"C0001\":          8194," \
        "            \"C0002\":         10005," \
        "            \"C0006\":           177," \
        "            \"C0008\":          8853," \
        "            \"C0009\":          8854," \
        "            \"C0015\":          8226," \
        "            \"C0024\":           126," \
        "            \"C0033\":          8594," \
        "            \"C0035\":          8595," \
        "            \"C0036\":          8596," \
        "            \"_comment_\": [ \"missing code for\",              9633 ]" \
        "        }," \
        "        \"zapfdingbatsMap\":" \
        "        {" \
        "            \"a12\":            9758," \
        "            \"a19\":           10003," \
        "            \"a26\":           10010," \
        "            \"a35\":            9733," \
        "            \"a72\":           10061," \
        "            \"a46\":           10035," \
        "            \"a54\":           10043," \
        "            \"a56\":           10038," \
        "            \"a66\":           10055," \
        "            \"a67\":           10056," \
        "            \"a71\":            9679," \
        "            \"a73\":            9632," \
        "            \"a74\":           10063," \
        "            \"a76\":            9650," \
        "            \"a77\":            9660," \
        "            \"a78\":            9670," \
        "            \"a83\":           10073," \
        "            \"a111\":           9670," \
        "            \"a140\":           9312," \
        "            \"a141\":           9313," \
        "            \"a142\":           9314," \
        "            \"a143\":           9315," \
        "            \"a144\":           9316," \
        "            \"a145\":           9317," \
        "            \"a146\":           9318," \
        "            \"a147\":           9319," \
        "            \"a148\":           9320," \
        "            \"a149\":           9321," \
        "            \"a150\":          10122," \
        "            \"a151\":          10123," \
        "            \"a160\":           8594," \
        "            \"a161\":           8594," \
        "            \"a163\":           8596," \
        "            \"a167\":          10140," \
        "            \"a169\":          10142," \
        "            \"a170\":          10143," \
        "            \"a174\":          10148," \
        "            \"uni25A0\":        9632," \
        "            \"uni25CF\":        9679," \
        "            \"uni2759\":       10073" \
        "        }," \
        "        \"universalGreekMathPiMap\":" \
        "        {" \
        "            \"_comment_\": \"univ greek + mathematicalPi have its own set of codes, sometimes used in one font. See: https://blogs.ch.cam.ac.uk/pmr/2012/11/\"," \
        "            \"H9024\":           937," \
        "            \"H9251\":           945," \
        "            \"H9252\":           946," \
        "            \"H9253\":           611," \
        "            \"H9254\":           948," \
        "            \"H9255\":           949," \
        "            \"H9256\":           950," \
        "            \"H9257\":           951," \
        "            \"H9258\":           952," \
        "            \"H9259\":          8489," \
        "            \"H9260\":           954," \
        "            \"H9261\":           955," \
        "            \"H9262\":           956," \
        "            \"H11001\":           43," \
        "            \"H11002\":           45," \
        "            \"H11003\":          215," \
        "            \"H11004\":          247," \
        "            \"H11005\":           61," \
        "            \"H11006\":          177," \
        "            \"H11011\":          126," \
        "            \"H11017\":         8806," \
        "            \"H11021\":           60," \
        "            \"H11022\":           62," \
        "            \"H11032\":           39," \
        "            \"H11034\":          176," \
        "            \"H11554\":          183," \
        "            \"H11270\":        10913," \
        "            \"H11271\":        10914," \
        "            \"H17009\":         9650," \
        "            \"H17010\":         9660," \
        "            \"H17012\":         9654," \
        "            \"H17034\":         9711," \
        "            \"H17053\":         8853," \
        "            \"H17054\":         8854" \
        "        }," \
        "        \"universalNewsMap\":" \
        "        {" \
        "            \"T\":               174," \
        "            \"H11032\":           39," \
        "            \"H11033\":           34," \
        "            \"H11034\":          176," \
        "            \"H11542\":           34," \
        "            \"H17075\":         9654," \
        "            \"H18528\":          183," \
        "            \"H18549\":         9633," \
        "            \"H20324\":         8678," \
        "            \"H20325\":         8680," \
        "            \"H22071\":         9711," \
        "            \"H22668\":         9756," \
        "            \"H22669\":         9758," \
        "            \"H22813\":         9733," \
        "            \"H23041\":          174," \
        "            \"H23006\":          174," \
        "            \"H23007\":         8482" \
        "        }," \
        "        \"swissMap\":" \
        "        {" \
        "            \"_comment_\": [ \"Some swiss fonts use dash-glyphs of different lengths\"," \
        "                             \"- other hyphen chars (http://www.fileformat.info/info/unicode/category/Pd/list.htm)\"," \
        "                             \"8213 horizontal bar ―\"," \
        "                             \"65123 small hyphen minus ﹣\"," \
        "                             \"65293 full width hyphen minus －\"," \
        "                             \"65112 small em dash ﹘\" ]," \
        "            \"C0032\":     32," \
        "            \"C0035\":     35," \
        "            \"C0036\":     36," \
        "            \"C0037\":     37," \
        "            \"C0038\":     38," \
        "            \"C0039\":     39," \
        "            \"C0040\":     40," \
        "            \"C0041\":     41," \
        "            \"C0042\":     42," \
        "            \"C0043\":     43," \
        "            \"C0044\":     44," \
        "            \"C0045\":  65293," \
        "            \"C0046\":     46," \
        "            \"C0047\":     47," \
        "            \"C0048\":     48, \"C0049\":    49, \"C0050\":    50, \"C0051\":    51, \"C0052\":    52, \"C0053\":    53, \"C0054\":    54, \"C0055\":    55, \"C0056\":    56, \"C0057\":    57," \
        "            \"C0058\":     58," \
        "            \"C0059\":     59," \
        "            \"C0065\":     65, \"C0066\":    66, \"C0067\":    67, \"C0068\":    68, \"C0069\":    69, \"C0070\":    70, \"C0071\":    71, \"C0072\":    72, \"C0073\":    73," \
        "            \"C0074\":     74, \"C0075\":    75, \"C0076\":    76, \"C0077\":    77, \"C0078\":    78, \"C0079\":    79, \"C0080\":    80, \"C0081\":    81, \"C0082\":    82," \
        "            \"C0083\":     83, \"C0084\":    84, \"C0085\":    85, \"C0086\":    86, \"C0087\":    87, \"C0088\":    88, \"C0089\":    89, \"C0090\":    90," \
        "            \"C0091\":     91, \"C0093\":    93," \
        "            \"C0095\":     95," \
        "            \"C0097\":     97, \"C0098\":    98, \"C0099\":    99, \"C0100\":   100, \"C0101\":   101, \"C0102\":   102, \"C0103\":   103, \"C0104\":   104, \"C0105\":   105," \
        "            \"C0106\":    106, \"C0107\":   107, \"C0108\":   108, \"C0109\":   109, \"C0110\":   110, \"C0111\":   111, \"C0112\":   112, \"C0113\":   113, \"C0114\":   114," \
        "            \"C0115\":    115, \"C0116\":   116, \"C0117\":   117, \"C0118\":   118, \"C0119\":   119, \"C0120\":   120, \"C0121\":   121, \"C0122\":   122," \
        "            \"C0123\":    123, \"C0124\":   124, \"C0125\":   125," \
        "            \"C0176\":    176," \
        "            \"C0262\":     45" \
        "        }" \
        "    }" \
        "}";
}
