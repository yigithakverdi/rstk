#!/bin/bash

echo "Creating a new Google Cloud Compute instance named 'rstk-simulation'..."
echo "Setting project to 'rstk-436515'..."

gcloud config set project rstk-436515

echo "Creating the instance in zone 'europe-west3-a' with machine type 'e2-highmem-8'..."

gcloud compute instances create rstk-simulation --project=rstk-436515 \
                                                --zone=europe-west3-a \ 
                                                --machine-type=e2-highmem-8 \ 
                                                --network-interface=network-tier=PREMIUM,stack-type=IPV4_ONLY,subnet=default \
                                                --maintenance-policy=MIGRATE \
                                                --provisioning-model=STANDARD \
                                                 --service-account=1031293182645-compute@developer.gserviceaccount.com \
                                                 --scopes=https://www.googleapis.com/auth/devstorage.read_only,https://www.googleapis.com/auth/logging.write,https://www.googleapis.com/auth/monitoring.write,https://www.googleapis.com/auth/service.management.readonly,https://www.googleapis.com/auth/servicecontrol,https://www.googleapis.com/auth/trace.append \
                                                 --create-disk=auto-delete=yes,boot=yes,device-name=rstk-simulation,image=projects/debian-cloud/global/images/debian-12-bookworm-v20240910,mode=rw,size=10,type=pd-balanced \
                                                 --no-shielded-secure-boot \
                                                 --shielded-vtpm \
                                                 --shielded-integrity-monitoring \
                                                 --labels=goog-ec-src=vm_add-gcloud \
                                                 --reservation-affinity=any

echo "Instance 'rstk-simulation' created successfully."