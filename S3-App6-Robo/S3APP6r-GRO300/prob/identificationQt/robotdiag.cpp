#include "robotdiag.hpp"
#include "robotsim.hpp"

#include <vector>
#include <queue>
#include <cstdio>
#include <thread>
#include <mutex>
#include <condition_variable>

//le but est de partir un 2e fil qui exporte des donnes et ecrit un fichier
std::mutex mutex_;
std::condition_variable data_cond;
std::thread thread_record;

using namespace s3gro;


RobotDiag::RobotDiag()
{
    // Démarre le simulateur:
    // TODO: Supprimer cette ligne si vous testez avec un seul moteur
    robotsim::init(this, 8, 10, 3);   // Spécifie le nombre de moteurs à
                                      // simuler (8) et le délai moyen entre
                                      // les événements (10 ms) plus ou moins
                                      // un nombre aléatoire (3 ms).


}

// Le destructeur sera normalement appellé à la fermeture de l'application.
// Écrit des statistiques à l'écran.
RobotDiag::~RobotDiag() {
    stop_recording();
}

void RobotDiag::push_event(RobotState new_robot_state) {
    // Conserve toutes les données

    //On aura besoin de synchroniser queue_ avec export_loop (pour le CSV),
    //car on veut run export_loop dans un thread en parallele au thread principal qui run push_event.
    //On ne veut pas que plusieurs threads manipulent  les donnes de robot_state,
    //ou qu'ils rajoutent un new_robot_state a la queue_ en meme temps, alors on met unique_lock ou  lock_guard

    std::unique_lock<std::mutex> lock(mutex_);
    data_.push_back(new_robot_state);
    
    // Ajoute le dernier événement à la file d'exportation
    queue_.push(new_robot_state);
    data_cond.notify_one();
   //Notify_One est ici, car on veut notifier le export_loop que les donnees ont bel et bien ete rajoutes au queue.
   //Alors elles sont pretes a etre exportees
    
}

void RobotDiag::set_csv_filename(const std::string& file_name) {
    csv_filename_ = file_name;
}

void RobotDiag::start_recording() {
    // Indique que le système de diagnostic fonctionne (à mettre à 'false' lors
    // de la fermeture pour interrompre le fil d'exportation).

    run_ = true;

    // TODO : Lancement du fil.
    // start_recording() lance un thread qui exporte le data, afin doptimiser le temps du programme.
    // Alors on peut export data pendant que push_data run en parallel.

    
     thread_record = std::thread(&RobotDiag::export_loop, this);
}

void RobotDiag::stop_recording() {
    // Indique que le système de diagnostic doit être arrêté.

    run_ = false;

    // TODO : Fermeture du fil.
    
    thread_record.join();

    robotsim::stop_and_join();

    printf("Final vector size: %zu\n", data_.size());
}

// Fonction d'exportation vers CSV.
// Doit être exécutée dans un fil séparé et écrire dans le fichier CSV
// lorsque de nouvelles données sont disponibles dans queue_.

void RobotDiag::export_loop()
{

    if (csv_filename_.empty()) {
        csv_filename_ = "/tmp/robotdiag.csv";
    }

    FILE* out = fopen(csv_filename_.c_str(), "w");

    if (out == NULL) {
        printf("ERROR: Cannot open output file.\n");
        return;
    }

    // En-tête du fichier CSV, respectez le format.
    fprintf(out, "motor_id;t;pos;vel;cmd\n");

// ****************************************TODO: Synchronisation et écriture********************************

    while (run_== true)
    {
        std::unique_lock<std::mutex> lock(mutex_);// Utiliser unique_lock, car on a un condition_variable.
        // on veut unlock apres que datacond.wait a ete satisfait, on utilise pas un lock_guard car celui-ci ferme le thread jusqu'a sa destruction

        data_cond.wait(lock); // Thread attend que le thread principal ait stocké les informations dans queue_.
                              // Lorsque notify_one a été exécuté, on peut passer le wait condition_variable,
                              // apres que le wait soit satisfait, on peut acceder aux informations du queue_ et les ecrire dans le CSV

        if (!queue_.empty())
        {
            RobotState evt = queue_.front(); //On prend la premiere donnee de la liste queue_, elle deviendra le state evt
            if (evt.id == 0) // Le id du vrai moteur sur arduino est 0. 1 a infini sont des simulations
            {
                fprintf(out, "%i; %f; %f; %f; %f"  ,  evt.id, evt.t, evt.cur_pos, evt.cur_vel, evt.cur_cmd); //Respecter format
                queue_.pop(); // apres le fprintf, car on ne veut pas supprimer le data
            }
         }
     }
     fclose(out);
}

