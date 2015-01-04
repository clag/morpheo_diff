#include <QApplication>
#include <QFile>
#include <QTextStream>
#include <QSqlQueryModel>
#include <QSqlError>
#include <QSqlRecord>
#include <QVector>
#include <iostream>
#include <math.h>

#include <QSqlDatabase>

QSqlDatabase database;
QString fileNamev2 = "/home/clairel/Codes/build-Morpheo-Desktop-Release/dtopo_avignon_ARP_2014.txt";
QString fileNamev1 = "/home/clairel/Codes/build-Morpheo-Desktop-Release/dtopo_avignon_ARP_1970.txt";
QString name = "avignon_ARP";

bool connexion(){

    QString host = "localhost";
    QString user = "claire";
    QString pass = "claire";

    database = QSqlDatabase::addDatabase("QPSQL");

    database.setHostName(host);
    database.setDatabaseName(name);
    database.setUserName(user);
    database.setPassword(pass);

    return database.open();
}

void deconnexion(){
    database.close();
}


int main()
{
    // Année G2

    QFile fichierv2(fileNamev2);
    fichierv2.open(QIODevice::ReadOnly | QIODevice::Text);
    QTextStream fluxv2(&fichierv2);

    QVector< int > dtopo_ligne;

    QString mot;
    while(! fluxv2.atEnd())
    {
        fluxv2 >> mot;
        dtopo_ligne.push_back(mot.toInt());
        // traitement du mot
    }

    fichierv2.close();

    int size = dtopo_ligne.size();

    int nvoie_v2 = sqrt(size);

    int dtopo_matrice_g2[nvoie_v2+1][nvoie_v2+1];

    for (int i = 0; i < nvoie_v2; i++) {

        for (int j = 0; j < nvoie_v2; j++) {
            dtopo_matrice_g2[i+1][j+1] = dtopo_ligne.at(i*nvoie_v2 + j);
        }

    }

    // Année G1

    QFile fichierv1(fileNamev1);
    fichierv1.open(QIODevice::ReadOnly | QIODevice::Text);
    QTextStream fluxv1(&fichierv1);

    dtopo_ligne.clear();

    while(! fluxv1.atEnd())
    {
        fluxv1 >> mot;
        dtopo_ligne.push_back(mot.toInt());
        // traitement du mot
    }

    fichierv1.close();

    size = dtopo_ligne.size();

    int nvoie_v1 = sqrt(size);

    int dtopo_matrice_g1[nvoie_v1+1][nvoie_v1+1];

    for (int i = 0; i < nvoie_v1; i++) {

        for (int j = 0; j < nvoie_v1; j++) {
            dtopo_matrice_g1[i+1][j+1] = dtopo_ligne.at(i*nvoie_v1 + j);
        }

    }

    if (! connexion()) {
        std::cerr << "Impossible de se connecter à la base" << std::endl;
        return 1;
    }

    // Récupération de appariement

    QSqlQueryModel appariement;

    appariement.setQuery(QString("SELECT idv_g1, idv_g2, struct_g1, struct_g2, length FROM appariement;"));

    if (appariement.lastError().isValid()) {
        std::cerr << "Erreur récupération appariement : " << appariement.lastError().text().toStdString() << std::endl;
        return 1;
    }

    std::cout << "nombre d'appariement : " << appariement.rowCount() << std::endl;

    // Récupération de removal

    QSqlQueryModel removal;

    removal.setQuery(QString("SELECT idv_g1, struct_g1, length FROM removal;"));

    if (removal.lastError().isValid()) {
        std::cerr << "Erreur récupération removal : " << removal.lastError().text().toStdString() << std::endl;
        return 1;
    }

    std::cout << "nombre de suppression : " << removal.rowCount() << std::endl;

    // Récupération de addition

    QSqlQueryModel addition;

    addition.setQuery(QString("SELECT idv_g2, struct_g2, length FROM addition;"));

    if (addition.lastError().isValid()) {
        std::cerr << "Erreur récupération addition : " << addition.lastError().text().toStdString() << std::endl;
        return 1;
    }

    std::cout << "nombre d'addition : " << addition.rowCount() << std::endl;

    // Et on taffe
    for (int i = 0; i < appariement.rowCount(); i++) {
        int idv2 = appariement.record(i).value("idv_g2").toInt();
        int idv1 = appariement.record(i).value("idv_g1").toInt();

        float struct2 = appariement.record(i).value("struct_g2").toFloat();
        float struct1 = appariement.record(i).value("struct_g1").toFloat();

        // Les suppressions
        float sum_rem = 0;
        for (int j = 0; j < removal.rowCount(); j++) {
            int idv1r = removal.record(j).value("idv_g1").toInt();
            float lengthr = removal.record(j).value("length").toFloat();
            sum_rem += dtopo_matrice_g1[idv1][idv1r]*lengthr;
        }

        // Les ajouts
        float sum_add = 0;
        for (int j = 0; j < addition.rowCount(); j++) {
            int idv2a = addition.record(j).value("idv_g2").toInt();
            float lengtha = addition.record(j).value("length").toFloat();
            sum_add += dtopo_matrice_g2[idv2][idv2a]*lengtha;
        }

        float deltastruct = struct2 - struct1 + sum_rem - sum_add;

        std::cout << "Voie appariée " << idv2 << std::endl;
        std::cout << "     sum_rem = " << sum_rem << std::endl;
        std::cout << "     sum_add = " << sum_add << std::endl;
        std::cout << "     deltastruct = " << deltastruct << std::endl << std::endl;


        QSqlQueryModel update_appariement;

        update_appariement.setQuery(QString("UPDATE appariement SET delta_struct = %1 WHERE idv_g2 = %2;").arg(deltastruct).arg(idv2));

        if (update_appariement.lastError().isValid()) {
            std::cerr << "Erreur update appariement : " << update_appariement.lastError().text().toStdString() << std::endl;
            return 1;
        }
    }

    QSqlQueryModel update_appariement_rel;

    update_appariement_rel.setQuery(QString("UPDATE appariement SET delta_struct_rel = delta_struct / struct_g2;"));

    if (update_appariement_rel.lastError().isValid()) {
        std::cerr << "Erreur update_appariement_rel : " << update_appariement_rel.lastError().text().toStdString() << std::endl;
        return 1;
    }

    // Nettoyage
    deconnexion();

    return 0;
}


