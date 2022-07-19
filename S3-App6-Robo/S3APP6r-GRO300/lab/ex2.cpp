#include <cstdio>
#include <thread>
#include <mutex>
#include <queue>
#include <cstdlib>      // rand
#include <condition_variable>


namespace {
    std::queue<int> queue_;
    std::mutex      mutex_;
 
    std::condition_variable queue_cond; // on cree une condition variable qu'on utilisera dans un .wait
    
}

void add_to_queue(int v)
{
    // Fournit un accès synchronisé à queue_ pour l'ajout de valeurs.
    
    //std::lock_guard<std::mutex> lock(mutex_);
    queue_.push(v); 
    
    queue_cond.notify_one(); //Je veux: apres que ça ajoute 1 element au queue, on peut verifier cet élément. Notif_one unblock le wait dans consom(). 
}

void prod()
{
    // Produit 100 nombres aléatoires de 1000 à 2000 et les ajoute
    // à une file d'attente (queue_) pour traitement.
    // À la fin, transmet "0", ce qui indique que le travail est terminé.

    for (int i = 0; i < 100; ++i)
    {
        int r = rand() % 1001 + 1000;
        add_to_queue(r);

        // Bloque le fil pour 50 ms:
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    add_to_queue(0);
    
}




void cons()
{
    while (true)
    {
        std::unique_lock<std::mutex> lock(mutex_); //mettre en unique car on utilise un condition variable juste en dessous
        
        
        // On doit toujours vérifier si un objet std::queue n'est pas vide avant de retirer un élément.

        queue_cond.wait(lock, []{return !queue_.empty();}); 
  
        // on enleve le if(!queue empty) car on regardera deja cette condition avec le .wait. Si la queue n'est pas vide, la condition est satisfaite et notify_one peut débloquer le wait. 
            
            
            
            
         int v = queue_.front(); // Copie le premier élément de la queue.
         queue_.pop();           // Retire le premier élément.

            //std::this_thread::sleep_for(std::chrono::milliseconds(10));
            
            
            
            printf("Reçu: %d\n", v);
            
            if (v == 0) 
            {
                // Travail terminé, on quitte la fonction du consommateur.
                return;
            }    
     }
}

int main(int argc, char** argv)
{
    std::thread t_prod(prod);
    std::thread t_cons(cons);

    t_prod.join();
    t_cons.join();

    return 0;
}

