Hi!

If you would like to do the translation of your language follow these steps:

(the LC symbols always mean you language code, eg: EN,HU,DE... )

1.  Download the latest source package, and copy the src/lang/start/cdcat_nolang.ts file
    to src/lang/cdcat_LC.ts.

        $ cp src/lang/start/cdcat_nolang.ts src/lang/cdcat_LC.ts

2.  If your linux box contains the "linguist" QT program run go to 3. step.
    Install the "linguist" QT program. It should be in QT package.

3.  Run the "linguist" under X  and open your cdcat_LC.ts file.

4.  Do the translation.

    Now the translation is done, if you don't like to use it immediately
    just send me the \*.ts files. But if you would like to use it in cdcat
    continue with the other steps.

5.  Open the src/cdcat.pro and append your language file to the necessary place.
    You will find it next to the other translations. (Find "cdcat_de.ts").

6.  Run "lrelease cdcat.pro". The lrelease program is part of the Qt.

7.  Rebuild and reinstall the program.

8.  Send me your translation, so I can put it to the next release.
