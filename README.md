# teiInternship

## V1 - (Tamamlandı)
- [x] Lisans dosyası şifrelenmiş formatta okunamaz olacak
- [x] Şifre çözüldüğünde JSON formatı görülecek
- [x] Şifre en az 8 karakter olacak ve büyük/küçük harf, özel karakter içerecek
- [x] Lisans dosyası formatı `.lic` olarak güncellenecek
- [x] Kullanıcı adı en az 6 karakter olacak

## V2 
- [x] Password hash'lenip JSON'a o şekilde yazılacak
- [x] Asimetrik bir şifreleme algoritması araştırılıp karar verilecek (sebepleriyle birlikte)
- [x] Tüm JSON içeriğinin hash'i alınacak
- [x] Rastgele bir asimetrik key çifti oluşturulacak. *Private key* sadece lisans uygulamasında, *Public key* login uygulamasında bulunacak.
- [x] Lisans uygulamasında private ile hash imzalanacak
- [ ] Login uygulamasında lisansın imzası Public key ile çözülecek, daha sonra lisans hash'i ile karşılaştırılacak
- [x] Private key'in projede nasıl tutulacağı araştırılacak
