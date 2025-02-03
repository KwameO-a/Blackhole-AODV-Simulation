# **Blackhole Attack Simulation in AODV**

## **Overview**
This project simulates a **Blackhole Attack** in an **AODV (Ad hoc On-Demand Distance Vector Routing)** network using **NS-3.45**. The implementation includes a custom BlackholeAODV module and mitigation strategies using trust-based detection and blacklisting.

---

## **Features**
- **Blackhole Attack Implementation**: A malicious node drops packets instead of forwarding them.
- **Trust-based Mitigation**: Nodes assign trust scores to their neighbors based on packet forwarding behavior.
- **Blacklist Mechanism**: Nodes with low trust scores are blacklisted to prevent further routing through them.
- **Packet Drop and Forward Tracking**: Tracks the number of forwarded and dropped packets.
- **Flow Monitoring and Logging**: Uses **NS-3.45's Flow Monitor** for performance analysis.

---

## **Tech Stack**
- **Simulation Framework:** NS-3.45
- **Routing Protocol:** Modified AODV
- **Programming Language:** C++

---

## **Installation & Compilation**

### **1. Clone the Repository**
```sh
git clone https://github.com/your-username/blackhole-aodv.git
```

### **2. Navigate to the Project Folder**
```sh
cd blackhole-aodv
```

### **3. Build the Project**
Compile the code using **NS-3.45’s build system**:
```sh
cd ns-3.45
./build.py --enable-examples --enable-tests
```
Or, compile specific files manually:
```sh
./ns3 build
```

---

## **Running the Simulation**
Run the executable:
```sh
./ns3 run blackhole
```

---
