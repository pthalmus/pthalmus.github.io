-- =============================================
-- Create date: <2025-08-06>
-- Description:	<Checking Correct Member>
-- =============================================
CREATE PROCEDURE Select_Member
	@ID varchar(30),
	@PW varchar(30)
AS
BEGIN
	SET NOCOUNT ON;

	declare @nCount INT;
	SELECT @nCount = Count(*) from dbo.Member where ID = @ID and PW = @PW

	if(@nCount = 1)
	BEGIN
		SELECT 1;
	END
	ELSE
	BEGIN
		SELECT 0;
	END
END
