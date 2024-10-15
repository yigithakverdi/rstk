To set up your testbed in Kathara and test RPKI based on the topology you provided, you'll need to strategically place certain images and services on the different routers to simulate a realistic environment for RPKI testing.

### 1. **Understand the Topology:**

- **R1** and **R2** are within AS12 and likely exchange routes internally.
- **R3** connects to **R1** and the Routinator for RPKI validation.
- **R4** and **R5** belong to different ASes (AS 8075 and AS 29256), so they will be external peers, which can be used for testing the enforcement of valid/invalid route advertisements.

### 2. **Components and Software for RPKI Testing:**

For RPKI, the essential components are:
- **RPKI Relying Party**: (Routinator in your case) which validates the ROAs (Route Origin Authorizations) published by the RPKI Certificate Authorities.
- **RPKI-enabled routers**: BGP routers that fetch the validated ROAs from the Routinator and enforce them by filtering invalid routes.

In your testbed:
- Use **Routinator** (already set up) as the RPKI Relying Party.
- Use **FRR** or **Quagga** on routers that need RPKI capabilities (not all routers need to be RPKI-enabled).
- BGP routers that don't directly handle RPKI but peer with routers that do (like R2 or R5) can use **FRRouting** or **OpenBGPD**.
  
### 3. **Assigning Images to Containers:**

- **Routinator**: Use the **kathara/routinator** image. You've already set up this service, and it will connect to R3, which will validate routes via RPKI.
- **R1, R2, R3**: Use **kathara/frr** or **kathara/quagga** since FRRouting and Quagga support BGP and can be configured for RPKI validation. I recommend **FRRouting** as it has more advanced features for RPKI validation. R3 will be the key router fetching ROAs from the Routinator.
- **R4** and **R5**: You can also use **kathara/frr** for these routers, but they don’t necessarily need to perform RPKI validation themselves. They can be set up as external BGP peers advertising routes to R3. If you want a different flavor of BGP, you can use **kathara/openbgpd** for variety.

### 4. **Configuring RPKI and BGP on the Routers:**

- **Routinator Configuration**:
  You’ve set up the Routinator, and it should be configured to fetch ROAs from a trust anchor (like ARIN or RIPE NCC).
  
- **R3 Configuration**:
  - Install the **FRRouting** software.
  - Configure RPKI validation using the following commands in the FRRouting shell:
    ```bash
    rpki
    rpki cache connection to [Routinator IP] port 3323
    ```
  - Enable BGP and configure peering with R1, R4, and R5:
    ```bash
    router bgp 8075
      neighbor [R1 IP] remote-as 12
      neighbor [R4 IP] remote-as 8075
      neighbor [R5 IP] remote-as 29256
    ```
  - Set up RPKI-based filtering:
    ```bash
    bgp rpki validation enable
    bgp bestpath prefix-validate allow-invalid
    bgp bestpath prefix-validate allow-valid
    ```

- **R1 and R2 Configuration**:
  These routers don’t need to perform RPKI validation themselves but should peer with R3 for BGP. Use the same **FRRouting** commands to configure basic BGP without RPKI.

- **R4 and R5 Configuration**:
  These routers can either use **FRRouting** or **OpenBGPD** and will act as external peers advertising routes to R3. You don’t need to enable RPKI on them if you only want R3 to validate routes.

### 5. **Routing and Testing:**

- Configure the BGP neighbors and test routing advertisements between the routers.
- From R3, test the RPKI validation by advertising invalid prefixes from R4 or R5 and ensuring that R3 rejects these invalid routes.
  
### 6. **Summary of Image Assignments:**

- **Routinator**: Use **kathara/routinator** (already set up).
- **R1, R2, R3**: Use **kathara/frr**. R3 will fetch ROAs from the Routinator and validate them.
- **R4, R5**: Use **kathara/frr** or **kathara/openbgpd**. They will act as external BGP peers.

This setup should give you a functional RPKI testbed. Let me know if you need more details on configuration steps!
