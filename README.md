A mini DRC rule engine that models a basic EDA flow.

note: "files.zip" have to unzip, and the layout*.txt file you can generate by yourself.

Demo ( just complie main.cpp ): 

https://github.com/user-attachments/assets/2aaa6999-f78f-4cf2-bf89-1c0f4f7d02dc

*Two input files*: 

1.layout*.txt (layout1~4)
   
Take layout1.txt for instance(unit in nm):

<img width="350" height="250" alt="image" src="https://github.com/user-attachments/assets/08d76889-804c-4d6b-b87d-d77c56824167" />

<img width="177" height="133" alt="image" src="https://github.com/user-attachments/assets/037cda5d-7a07-4118-af29-c61aed53dec0" />

2.rules.json 
this file will list some design rules

<img width="442" height="150" alt="image" src="https://github.com/user-attachments/assets/fb878f8a-0da3-4a21-a6ee-0dfc2a8ec61b" />


*Output*

The fail table (it won't report the correct part, only show the fail list).

This table summarizes all DRC violations for easy review.

To layout1.txt:

<img width="1323" height="243" alt="image" src="https://github.com/user-attachments/assets/3822d8c1-52d8-4dd6-af3c-552951c59ed5" />

"type" — The checking category. It shows which rule is being verified, such as WIDTH, SPACING, ENCLOSURE, or DENSITY.

"layer" — The metal or via layer where the check is applied (e.g., M1, M2, VIA12).

"object" — The target of the check.

idx=# refers to the index of the shape in the layout*.txt file.

(0,1) means the check involves two shapes: shape #0 and shape #1.
This notation only appears in SPACING checks.

"bbox" — The bounding box of the shape, describing its position from the lower-left to the upper-right corner.
For example, (0,0)-(20,100) means the shape spans from coordinates (0,0) to (20,100).

"rule" — The design rule threshold (e.g., >= 30, <= 120, >= 0.3).

"actual" — The measured or calculated value in the layout.

"delta" — The difference between the rule and the actual value.

"status" — The check result, either PASS or FAIL.

"window" — This term appears in DENSITY checks.
It represents a fixed-size analysis region (or “window”) used to calculate local metal density.
The coordinates shown in the object and bbox columns describe the window’s position on the die —
for example, object = [100,0] and bbox = (100,0)-(200,100) mean the density was evaluated in a 100 × 100 nm area starting at (100,0).

