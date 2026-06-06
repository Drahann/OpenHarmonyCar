const fs = require('fs');
const path = require('path');
const http = require('http');

const assetsDir = path.join(__dirname, 'assets');
if (!fs.existsSync(assetsDir)) {
  fs.mkdirSync(assetsDir, { recursive: true });
}

const fileMap = {
  // Medicines (1:848)
  "image2.png": "http://localhost:3845/assets/bd164a00a5d38114ef954845e922c272ff42e8a3.png",
  "image3.png": "http://localhost:3845/assets/730690a1bebe18c2578dfe0ad84720038f603f78.png",
  "image4.png": "http://localhost:3845/assets/f1dd56361df2adf02dff50e9177b4ef0d4c2f484.png",
  "image5.png": "http://localhost:3845/assets/73949487318648f1c3a71066fb3236215a664383.png",
  "image6.png": "http://localhost:3845/assets/159a526ae06ffcdbbab4e5dafa99b7bd68914e50.png",
  "image7.png": "http://localhost:3845/assets/7d9b0afc89e1e9096e3b5edfcf55353702039f6e.png",
  "image8.png": "http://localhost:3845/assets/d8678a02bd8c8e83e7f4e2a67b2f92d8ff7ab8a8.png",
  "shape.svg": "http://localhost:3845/assets/2a2e2557fae18502add87d9101fe210cc7210316.svg",
  "icon_cart.svg": "http://localhost:3845/assets/bfe27491193072ebcb22fd23b4932005dacbd9b5.svg",
  "oval.svg": "http://localhost:3845/assets/c3ff6a1993920714bad3c2b31235d4022b10a08b.svg",
  "icon_user.svg": "http://localhost:3845/assets/4942a110ad2551fde93d44bcea971a7585660197.svg",
  "search.svg": "http://localhost:3845/assets/44ed13acaad189f5d7634b7fa98b2990ce09b4b2.svg",
  "oval1.svg": "http://localhost:3845/assets/b177b6dc9eb0478ced66a1ce8709aba5eca15173.svg",
  "oval2.svg": "http://localhost:3845/assets/801725d47d686a35b69e4a9b31384453a1cce2da.svg",
  "circle.svg": "http://localhost:3845/assets/0caef0501c9c1d0dae0f5a7b915e9a7ba11e3fa8.svg",
  "arrow_icon.svg": "http://localhost:3845/assets/49bc2854a6c258d23524a853841fbdc46b87114f.svg",
  "battery.svg": "http://localhost:3845/assets/91ddfbd713b8a8b0f1f8af267f1457eae8d7aaee.svg",
  "wifi.svg": "http://localhost:3845/assets/1e9fe5545836442e7cf926ad02a009ed6d6a970e.svg",
  "cellular.svg": "http://localhost:3845/assets/fdc7f2e4e284cc9ae31c265bfc97e96e0adca8a4.svg",
  "fill1.svg": "http://localhost:3845/assets/bd302d8a250a02f5c806fd70ee0cbcf8b27266a3.svg",
  "group37056.svg": "http://localhost:3845/assets/f32fc57499323d27d947806d2df2a7b47d9e054b.svg",
  "group8.svg": "http://localhost:3845/assets/17c33773329facd87cf99cc0f2cf154b83186b4a.svg",
  "fill2.svg": "http://localhost:3845/assets/3bb671324fb382da2920c06c85b01e5433eee15f.svg",
  "fill3.svg": "http://localhost:3845/assets/c09d0f216234a7c54f7ed28f00a42459f5749ca3.svg",

  // Upload Prescription (1:742)
  "image_pres_ad.png": "http://localhost:3845/assets/b2dea3ad3bf6a12004d225e82f28037a1bdf5240.png",
  "shape_pres.svg": "http://localhost:3845/assets/ff6edbf4d326a66374c07c53bf73482f9bdfff98.svg",
  "frame18333.svg": "http://localhost:3845/assets/ca00b854e6fae6df672c01601bdc1cfe56ec50e6.svg",
  "icon_pres.svg": "http://localhost:3845/assets/1ad4a68b5923407e3097baa3012f11672038d6eb.svg",
  "icon_pres_1.svg": "http://localhost:3845/assets/06a8e4849150e83457a8a7a4414d68e6bdd53937.svg",
  "group7.svg": "http://localhost:3845/assets/d13d806775b780bc267e4cdeb37a24a60cc34a00.svg",
  "icon_pres_2.svg": "http://localhost:3845/assets/2dab576a1638a3d17c04989bd81dc7f8e049997e.svg",
  "oval_pres.svg": "http://localhost:3845/assets/46439d56eccbd8eacc3fe720813272d32f6cf426.svg",
  "arrow_pres.svg": "http://localhost:3845/assets/2521fa428afc9b18976ccba876fdc97991f727a1.svg",

  // Offers (1:1004)
  "shape_offers.svg": "http://localhost:3845/assets/d7f331b8684bc3be2d7c635c711f4b14a12565f5.svg",
  "bg_offers.svg": "http://localhost:3845/assets/92d941df30d8651f6d2a1e1a9b4e4843c340de04.svg",
  "frame18333_offers.svg": "http://localhost:3845/assets/0b77de1b1336780f8a972a75bdc67c15ede0fd67.svg",
  "icon_offers.svg": "http://localhost:3845/assets/30180b19cf4b78e3b5ae42257779356ab8abd9ca.svg",
  "search_offers.svg": "http://localhost:3845/assets/ff09e1be23a43be6cc09f48610654e29ffa06d1a.svg",
  "circle_offers.svg": "http://localhost:3845/assets/a4a1d0afb0623c39be3525acec5294952526c7b8.svg",
  "line3_offers.svg": "http://localhost:3845/assets/2df8c839e61cfb9f9746bdd31efd7031c92835eb.svg",
  "circle_offers_1.svg": "http://localhost:3845/assets/e58042a51f39aaea4d281b0af0140184550838aa.svg",
  "circle_offers_2.svg": "http://localhost:3845/assets/3b3a1653f76c8fac84a455cce5eae9d78b057b56.svg",
  "circle_offers_3.svg": "http://localhost:3845/assets/59a746fec71d80dd490186aec256f61a8ef1e8fd.svg",

  // Profile (1:1184)
  "shape_profile.svg": "http://localhost:3845/assets/a8b79a3ece0070827e76652f28d2a7e56f1113f1.svg",
  "frame18333_profile.svg": "http://localhost:3845/assets/6b7748f8543e8e09bff863329fc93cda40d5cc48.svg",
  "bg_profile.svg": "http://localhost:3845/assets/2a3c4131d5b763eb37b639be8321e5491bab0ab2.svg",
  "base_profile.svg": "http://localhost:3845/assets/3ad02372847125b433710c96d704185a282eac4d.svg",
  "mask_profile.svg": "http://localhost:3845/assets/8cdd0a35af7d27adc441afcf535b4b09335ca7c0.svg",
  "icon_profile.svg": "http://localhost:3845/assets/bb725cb23c8795df1981ae614202ffde9ce2c283.svg",
  "frame18339_profile.svg": "http://localhost:3845/assets/364d9ea89c5083c847a1166fcad65727ecc6acc3.svg",
  "line3_profile.svg": "http://localhost:3845/assets/d925a86d0e16cb9dced5a8e92497770440edc486.svg",
  "icon_profile_1.svg": "http://localhost:3845/assets/68dd398a25078f247cedc68f7ed970dcc5c4ef0e.svg",
  "icon_profile_2.svg": "http://localhost:3845/assets/6ad0c59ceb147d3429a0a6a56cd1c7bebf0f070d.svg",
  "group37061_profile.svg": "http://localhost:3845/assets/893ca0a1df81486ae352d33096805b9810f1830b.svg",
  "frame18340_profile.svg": "http://localhost:3845/assets/e60a5ed7667cea77522da07a744326a2e0f87eea.svg",
  "frame18341_profile.svg": "http://localhost:3845/assets/88a08145956f1c8b0a8953610b1f6b7405426d1e.svg",
  "icon_profile_3.svg": "http://localhost:3845/assets/4b752f778019ce511eefd30a2996e0bca4b55d2c.svg",
  "group37062_profile.svg": "http://localhost:3845/assets/c5400bf72bc9bb9dca925b96e21b39cb25ce44f6.svg",
  "group37064_profile.svg": "http://localhost:3845/assets/8686fe0991c2d3d9610fb959969a0633fbcd91f9.svg",
  "line4_profile.svg": "http://localhost:3845/assets/eb01dd2373861dfe29c737cc16202a9111209a81.svg"
};

async function downloadFile(name, url) {
  return new Promise((resolve, reject) => {
    const filePath = path.join(assetsDir, name);
    const file = fs.createWriteStream(filePath);
    http.get(url, (response) => {
      if (response.statusCode !== 200) {
        reject(new Error(`Failed to download ${name}, status: ${response.statusCode}`));
        return;
      }
      response.pipe(file);
      file.on('finish', () => {
        file.close();
        console.log(`Downloaded: ${name}`);
        resolve();
      });
    }).on('error', (err) => {
      fs.unlink(filePath, () => {});
      reject(err);
    });
  });
}

async function main() {
  const promises = Object.entries(fileMap).map(([name, url]) => downloadFile(name, url));
  try {
    await Promise.all(promises);
    console.log("All assets downloaded successfully.");
  } catch (error) {
    console.error("Asset download failed:", error);
    process.exit(1);
  }
}

main();
