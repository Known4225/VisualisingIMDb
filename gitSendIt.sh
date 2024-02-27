git add .
echo -n "message: "
read commitMessage
git commit -m "$commitMessage"
git push